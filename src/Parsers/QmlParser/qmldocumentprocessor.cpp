/**************************************************************************
**
** Copyright (c) 2026 Marcel Petrick
**
** This file is part of the SpellChecker Plugin, a Qt Creator plugin.
**
** The SpellChecker Plugin is free software: you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public License as
** published by the Free Software Foundation, either version 3 of the
** License, or (at your option) any later version.
**
** The SpellChecker Plugin is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with the SpellChecker Plugin.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include "qmldocumentprocessor.h"

#include <QRegularExpression>

using namespace SpellChecker;
using namespace SpellChecker::QmlSpellChecker::Internal;

namespace {

/*! \brief Line and column while scanning source text.
 *
 * Both values are one-based to match Qt Creator editor line and column
 * positions used elsewhere by the plugin.
 */
struct SourcePosition
{
  int line = 1;
  int column = 1;
};

/*! \brief QML text range that should be tokenized into words. */
struct Token
{
  /*! \brief Kind of source text that produced the token. */
  enum class Type {
    Comment, /*!< QML line or block comment. */
    Literal  /*!< QML string or template literal. */
  };

  Type type;    /*!< Token type. */
  QString text; /*!< Token source text. */
  int start = 0;/*!< Start offset in the original source. */
  int end = 0;  /*!< End offset in the original source. */
};

/*! \brief Advance a one-based source position by one character.
 * \param[in] c Character consumed.
 * \param[inout] position Position to update. */
void advancePosition( const QChar c, SourcePosition& position )
{
  if( c == QLatin1Char( '\n' ) ) {
    ++position.line;
    position.column = 1;
  } else {
    ++position.column;
  }
}

/*! \brief Query whether a quote at \a index is escaped.
 * \param[in] source Source text.
 * \param[in] index Index of the quote to inspect.
 * \return true when the quote is preceded by an odd number of backslashes. */
bool isEscaped( const QString& source, int index )
{
  int backslashCount = 0;
  for( int i = index - 1; ( i >= 0 ) && ( source.at( i ) == QLatin1Char( '\\' ) ); --i ) {
    ++backslashCount;
  }
  return ( backslashCount % 2 ) == 1;
}

/*! \brief Query whether a character can be part of a raw word candidate.
 *
 * URL and email punctuation is included here so those strings can be extracted
 * as one candidate and then filtered out by dedicated false-positive filters.
 * \param[in] c Character to classify.
 * \return true when the scanner should keep reading the current candidate.
 */
bool isWordCharacter( const QChar c )
{
  return c.isLetterOrNumber()
         || ( c == QLatin1Char( '_' ) )
         || ( c == QLatin1Char( '.' ) )
         || ( c == QLatin1Char( '@' ) )
         || ( c == QLatin1Char( '\'' ) )
         || ( c == QLatin1Char( ':' ) )
         || ( c == QLatin1Char( '/' ) )
         || ( c == QLatin1Char( '?' ) )
         || ( c == QLatin1Char( '=' ) )
         || ( c == QLatin1Char( '#' ) )
         || ( c == QLatin1Char( '%' ) )
         || ( c == QLatin1Char( '-' ) );
}

/*! \brief Query whether a character should be trimmed from word edges.
 * \param[in] c Character to inspect.
 * \return true for punctuation that should not be spell checked as part of a word. */
bool isEdgePunctuation( const QChar c )
{
  return ( c.isLetterOrNumber() == false ) && ( c != QLatin1Char( '\'' ) );
}

/*! \brief Query whether a word candidate is a number or color.
 * \param[in] word Candidate text.
 * \return true if the candidate should be ignored as numeric/color data. */
bool isNumberOrColor( const QString& word )
{
  static const QRegularExpression doubleRe( QStringLiteral( "\\A\\d+(\\.\\d+)?\\z" ) );
  static const QRegularExpression hexRe( QStringLiteral( "\\A0x[0-9A-Fa-f]+\\z" ) );
  static const QRegularExpression colorRe( QStringLiteral( "\\A#?([0-9A-Fa-f]{2}){3,4}\\z" ) );
  return doubleRe.match( word ).hasMatch()
         || hexRe.match( word ).hasMatch()
         || colorRe.match( word ).hasMatch();
}

/*! \brief Query whether a word candidate is an email address.
 * \param[in] word Candidate text.
 * \return true if the candidate looks like an email address. */
bool isEmailAddress( const QString& word )
{
  static const QRegularExpression emailRe( QStringLiteral( "\\A[\\w\\-\\.]+@((?:[\\w]+\\.)+)[a-zA-Z]{2,}\\z" ) );
  return emailRe.match( word ).hasMatch();
}

/*! \brief Query whether a word candidate is a website address.
 * \param[in] word Candidate text.
 * \return true if the candidate looks like a website address. */
bool isWebsite( const QString& word )
{
  static const QRegularExpression websiteRe( QStringLiteral( "\\A((http(s)?):\\/\\/)?(\\w+\\.){1,10}[0-9A-Za-z./?=#%\\-]+\\z" ) );
  return websiteRe.match( word ).hasMatch();
}

/*! \brief Split camelCase words using the same rough rule as the C++ parser.
 * \param[in] word Word candidate to split.
 * \return Split words, or the original word when no camelCase boundary is found. */
QStringList splitCamelCase( const QString& word )
{
  static const QRegularExpression camelCaseContainsRe( QStringLiteral( "[a-z]{1,}[A-Z]{1,}[a-z]{1,}" ) );
  static const QRegularExpression camelCaseIndexRe( QStringLiteral( "[a-z][A-Z]" ) );
  if( word.contains( camelCaseContainsRe ) == false ) {
    return { word };
  }

  QStringList splitWords;
  QList<int> indexes;
  indexes << 0;
  int lastIdx = 0;
  while( true ) {
    const int currentIdx = word.indexOf( camelCaseIndexRe, lastIdx );
    if( currentIdx == -1 ) {
      indexes << word.length();
      break;
    }
    lastIdx = currentIdx + 1;
    indexes << lastIdx;
  }

  for( int idx = 0; idx < indexes.count() - 1; ++idx ) {
    splitWords << word.mid( indexes.at( idx ), indexes.at( idx + 1 ) - indexes.at( idx ) );
  }
  return splitWords;
}

/*! \brief Convert string split results back to positioned Word objects.
 * \param[in] stringList Split word texts.
 * \param[in] word Original word that was split.
 * \return Word objects with adjusted line, column and source offsets. */
WordList wordsFromSplitString( const QStringList& stringList, const Word& word )
{
  WordList wordList;
  int currentPos = 0;
  for( const QString& text: stringList ) {
    Word newWord;
    newWord.text = text;
    newWord.fileName = word.fileName;
    currentPos = word.text.indexOf( newWord.text, currentPos );
    newWord.columnNumber = word.columnNumber + currentPos;
    newWord.lineNumber = word.lineNumber;
    newWord.length = newWord.text.length();
    newWord.start = word.start + currentPos;
    newWord.end = newWord.start + newWord.length;
    newWord.charAfter = word.charAfter;
    newWord.inComment = word.inComment;
    currentPos += newWord.length;
    wordList.append( newWord );
  }
  return wordList;
}

/*! \brief Apply QML parser filters and append a word if it should be checked.
 *
 * This handles common false positives and recursively splits words on numbers,
 * underscores, dots and camelCase boundaries.
 * \param[in] word Candidate word.
 * \param[in] settings Parser settings.
 * \param[inout] words Destination list for accepted words. */
void appendWordWithSplits( const Word& word, const QmlParserSettings& settings, WordList& words )
{
  const QString currentWord = word.text;
  const QString currentWordCaps = currentWord.toUpper();

  if( isNumberOrColor( currentWord ) ) {
    return;
  }
  if( ( settings.removeEmailAddresses == true ) && isEmailAddress( currentWord ) ) {
    return;
  }
  if( ( settings.removeWebsites == true ) && isWebsite( currentWord ) ) {
    return;
  }
  if( ( settings.checkAllCapsWords == false ) && ( currentWord == currentWordCaps ) ) {
    return;
  }

  static const QRegularExpression splitRe( QStringLiteral( "[0-9_\\.]+" ) );
  const QStringList pieces = currentWord.split( splitRe, Qt::SkipEmptyParts );
  if( pieces.count() > 1 ) {
    WordList splitWords = wordsFromSplitString( pieces, word );
    for( const Word& splitWord: splitWords ) {
      appendWordWithSplits( splitWord, settings, words );
    }
    return;
  }

  const QStringList camelCaseWords = splitCamelCase( currentWord );
  if( camelCaseWords.count() > 1 ) {
    WordList splitWords = wordsFromSplitString( camelCaseWords, word );
    for( const Word& splitWord: splitWords ) {
      appendWordWithSplits( splitWord, settings, words );
    }
    return;
  }

  words.append( word );
}

/*! \brief Extract positioned words from one token.
 * \param[in] fileName File name to assign to each word.
 * \param[in] source Full source text.
 * \param[in] token Token to split into words.
 * \param[in] settings Parser settings.
 * \param[inout] words Destination list for accepted words. */
void extractWordsFromToken( const QString& fileName, const QString& source, const Token& token, const QmlParserSettings& settings, WordList& words )
{
  SourcePosition position;
  for( int idx = 0; idx < token.start; ++idx ) {
    advancePosition( source.at( idx ), position );
  }

  int wordStart = -1;
  SourcePosition wordPosition = position;
  for( int idx = token.start; idx <= token.end; ++idx ) {
    const bool atEnd = idx >= token.end;
    const QChar c = atEnd ? QLatin1Char( ' ' ) : source.at( idx );
    const bool wordCharacter = ( atEnd == false ) && isWordCharacter( c );

    if( wordCharacter && ( wordStart < 0 ) ) {
      wordStart = idx;
      wordPosition = position;
    }

    if( ( wordStart >= 0 ) && ( wordCharacter == false ) ) {
      int normalizedStart = wordStart;
      int normalizedEnd = idx;
      while( ( normalizedStart < normalizedEnd ) && isEdgePunctuation( source.at( normalizedStart ) ) ) {
        ++normalizedStart;
      }
      while( ( normalizedEnd > normalizedStart ) && isEdgePunctuation( source.at( normalizedEnd - 1 ) ) ) {
        --normalizedEnd;
      }
      if( normalizedStart >= normalizedEnd ) {
        wordStart = -1;
        if( atEnd == false ) {
          advancePosition( c, position );
        }
        continue;
      }

      Word word;
      word.fileName = fileName;
      word.text = source.mid( normalizedStart, normalizedEnd - normalizedStart );
      word.start = normalizedStart;
      word.end = normalizedEnd;
      word.length = normalizedEnd - normalizedStart;
      word.charAfter = atEnd ? QLatin1Char( ' ' ) : c;
      word.inComment = ( token.type == Token::Type::Comment );
      word.lineNumber = wordPosition.line;
      word.columnNumber = wordPosition.column + ( normalizedStart - wordStart );
      appendWordWithSplits( word, settings, words );
      wordStart = -1;
    }

    if( atEnd == false ) {
      advancePosition( c, position );
    }
  }
}

/*! \brief Scan QML source for comments and string literals.
 * \param[in] source Full QML source text.
 * \param[in] settings Parser settings controlling which token types are emitted.
 * \return Token ranges to split into words. */
QVector<Token> scanTokens( const QString& source, const QmlParserSettings& settings )
{
  QVector<Token> tokens;
  const int length = source.length();

  for( int i = 0; i < length; ) {
    const QChar c = source.at( i );
    const QChar next = ( i + 1 < length ) ? source.at( i + 1 ) : QLatin1Char( '\0' );

    if( ( settings.whatToCheck.testFlag( QmlParserSettings::CheckComments ) == true )
        && ( c == QLatin1Char( '/' ) )
        && ( next == QLatin1Char( '/' ) ) ) {
      const int start = i + 2;
      i += 2;
      while( ( i < length ) && ( source.at( i ) != QLatin1Char( '\n' ) ) ) {
        ++i;
      }
      tokens.append( Token{ Token::Type::Comment, source.mid( start, i - start ), start, i } );
      continue;
    }

    if( ( settings.whatToCheck.testFlag( QmlParserSettings::CheckComments ) == true )
        && ( c == QLatin1Char( '/' ) )
        && ( next == QLatin1Char( '*' ) ) ) {
      const int start = i + 2;
      i += 2;
      bool closed = false;
      while( i + 1 < length ) {
        if( ( source.at( i ) == QLatin1Char( '*' ) ) && ( source.at( i + 1 ) == QLatin1Char( '/' ) ) ) {
          const int end = i;
          i += 2;
          tokens.append( Token{ Token::Type::Comment, source.mid( start, end - start ), start, end } );
          closed = true;
          break;
        }
        ++i;
      }
      if( closed == false ) {
        tokens.append( Token{ Token::Type::Comment, source.mid( start ), start, length } );
        i = length;
      }
      continue;
    }

    if( ( settings.whatToCheck.testFlag( QmlParserSettings::CheckStringLiterals ) == true )
        && ( ( c == QLatin1Char( '"' ) ) || ( c == QLatin1Char( '\'' ) ) || ( c == QLatin1Char( '`' ) ) ) ) {
      const QChar quote = c;
      const int start = i + 1;
      ++i;
      bool closed = false;
      while( i < length ) {
        if( ( source.at( i ) == quote ) && ( isEscaped( source, i ) == false ) ) {
          const int end = i;
          ++i;
          tokens.append( Token{ Token::Type::Literal, source.mid( start, end - start ), start, end } );
          closed = true;
          break;
        }
        ++i;
      }
      if( closed == false ) {
        tokens.append( Token{ Token::Type::Literal, source.mid( start ), start, length } );
      }
      continue;
    }

    ++i;
  }

  return tokens;
}

} // namespace

QmlDocumentProcessor::QmlDocumentProcessor( const QString& fileName, const QString& source, const QmlParserSettings& settings )
  : QObject( nullptr )
  , m_fileName( fileName )
  , m_source( source )
  , m_settings( settings )
{}

QmlDocumentProcessor::~QmlDocumentProcessor() = default;

void QmlDocumentProcessor::process( QmlDocumentProcessor::Promise& promise )
{
  if( promise.isCanceled() == true ) {
    promise.future().cancel();
    return;
  }

  promise.addResult( parseSource( m_fileName, m_source, m_settings ) );
}

WordList QmlDocumentProcessor::parseSource( const QString& fileName, const QString& source, const QmlParserSettings& settings )
{
  WordList words;
  const QVector<Token> tokens = scanTokens( source, settings );
  for( const Token& token: tokens ) {
    extractWordsFromToken( fileName, source, token, settings, words );
  }
  return words;
}
