/**************************************************************************
**
** Copyright (c) 2018 Carel Combrink
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

#include "cppdocumentparser.h"
#include "cppdocumentprocessor.h"
#include "cppparserconstants.h"

#include <cplusplus/Overview.h>
#include <cppeditor/cppeditordocument.h>
#include <cpptools/cppdoxygen.h>

using namespace SpellChecker;
using namespace SpellChecker::CppSpellChecker::Internal;

/*! \brief Testing assert that should be used during debugging
 * but should not be made part of a release. */
// #define SP_CHECK( test ) QTC_CHECK( test )
#define SP_CHECK( test )

class SpellChecker::CppSpellChecker::Internal::CppDocumentProcessorPrivate
{
public:
  CPlusPlus::Document::Ptr docPtr;
  HashWords tokenHashes;
  CppParserSettings settings;
  CPlusPlus::TranslationUnit* trUnit;
  QString fileName;

  CppDocumentProcessorPrivate( CPlusPlus::Document::Ptr documentPointer, const HashWords& hashWords, const CppParserSettings& cppSettings );
};
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

CppDocumentProcessorPrivate::CppDocumentProcessorPrivate( CPlusPlus::Document::Ptr documentPointer, const HashWords& hashWords, const CppParserSettings& cppSettings )
  : docPtr( documentPointer )
  , tokenHashes( hashWords )
  , settings( cppSettings )
  , trUnit( documentPointer->translationUnit() )
  , fileName( documentPointer->fileName() )
{}
// --------------------------------------------------

CppDocumentProcessor::CppDocumentProcessor( CPlusPlus::Document::Ptr documentPointer, const HashWords& hashWords, const CppParserSettings& cppSettings )
  : QObject( nullptr )
  , d( new CppDocumentProcessorPrivate( documentPointer, hashWords, cppSettings ) )
{
  d->docPtr->keepSourceAndAST();
}
// --------------------------------------------------

CppDocumentProcessor::~CppDocumentProcessor()
{
  if( d->docPtr != nullptr ) {
    d->docPtr->releaseSourceAndAST();
  }
}
// --------------------------------------------------

void CppDocumentProcessor::process( CppDocumentProcessor::FutureIF& future )
{
  SP_CHECK( docPtr.isNull() == false );
  SP_CHECK( trUnit != nullptr );
  QStringSet wordsInSource;
  QVector<WordTokens> wordTokens;
  /* If the setting is set to remove words from the list based on words found in the source,
   * parse the source file and then remove all words found in the source files from the list
   * of words that will be checked. */
  if( d->settings.removeWordsThatAppearInSource == true ) {
    /* First get all words that does appear in the current source file. These words only
     * include variables and their types */
    wordsInSource = getWordsThatAppearInSource();
  }

  if( future.isCanceled() == true ) {
    future.reportCanceled();
    return;
  }

  if( d->settings.whatToCheck.testFlag( CppParserSettings::CheckStringLiterals ) == true ) {
    /* Parse string literals */
    unsigned int tokenCount = d->trUnit->tokenCount();
    for( unsigned int idx = 0; idx < tokenCount; ++idx ) {
      const CPlusPlus::Token& token = d->trUnit->tokenAt( idx );
      if( token.isStringLiteral() == true ) {
        if( token.expanded() == true ) {
          /* Expanded literals comes from macros. These are not checked since they can be the
           * result of a macro like '__LINE__'. A user is not interested in such literals.
           * The input arguments to Macros are more usable like MY_MAC("Some String").
           * The macro arguments are checked after the normal literals are checked. */
          continue;
        }

        /* The String Literal is not expanded thus handle it like a comment is handled. */
        WordTokens tokens = parseToken( token, WordTokens::Type::Literal );
        wordTokens.append( tokens );
      }
    }
    /* Parse macros */
    wordTokens += parseMacros();
  }

  if( future.isCanceled() == true ) {
    future.reportCanceled();
    return;
  }

  if( d->settings.whatToCheck.testFlag( CppParserSettings::CheckComments ) == true ) {
    /* Parse comments */
    unsigned int commentCount = d->trUnit->commentCount();
    for( unsigned int comment = 0; comment < commentCount; ++comment ) {
      const CPlusPlus::Token& token = d->trUnit->commentAt( comment );
      /* Check to see if the current comment type must be checked */
      if( ( d->settings.commentsToCheck.testFlag( CppParserSettings::CommentsC ) == false )
          && ( token.kind() == CPlusPlus::T_COMMENT ) ) {
        /* C Style comments should not be checked and this is one */
        continue;
      }
      if( ( d->settings.commentsToCheck.testFlag( CppParserSettings::CommentsCpp ) == false )
          && ( token.kind() == CPlusPlus::T_CPP_COMMENT ) ) {
        /* C++ Style comments should not be checked and this is one */
        continue;
      }

      WordTokens::Type type = WordTokens::Type::Comment;
      if( ( token.kind() == CPlusPlus::T_DOXY_COMMENT )
          || ( token.kind() == CPlusPlus::T_CPP_DOXY_COMMENT ) ) {
        type = WordTokens::Type::Doxygen;
      }
      const WordTokens tokens = parseToken( token, type );
      wordTokens.append( tokens );
    }
  }

  if( future.isCanceled() == true ) {
    future.reportCanceled();
    return;
  }

  /* At this point the DocPtr can be released since it will no longer be
   * Used */
  d->docPtr->releaseSourceAndAST();
  d->docPtr.reset();

  // ----------------------------------
  /* Make a local copy of the last list of hashes. A local copy is made and used
   * as the input the tokenize function, but a new list is returned from the
   * tokenize function. If this is not done the list of hashes can grow forever
   * and cause a huge increase in memory. Doing it this way ensure that the
   * list only contains hashes of tokens that are present in during the last run
   * and will not contain old and invalid hashes. It will cause the parsing of a
   * different file than the previous run to be less efficient but if a file is
   * parsed multiple times, one after the other, it will result in a large speed up
   * and this will mostly be the case when editing a file. For this reason the initial
   * project parse on start up can be slower. */

  /* Populate the list of hashes from the tokens that was processed. */
  HashWords newHashesOut;
  WordList  newSettingsApplied;
  for( const WordTokens& token: qAsConst( wordTokens ) ) {
    WordList words = token.words;
    if( token.newHash == true ) {
      /* The words are new, they were not known in a previous hash
       * thus the settings must now be applied.
       * Only words that have already been checked against the settings
       * gets added to the hash, thus there is no need to apply the settings
       * again, since this will only waste time. */
      CppDocumentParser::applySettingsToWords( d->settings, token.string, wordsInSource, words );
    }
    newSettingsApplied.append( words );
    SP_CHECK( token.hash != 0x00 );
    newHashesOut[token.hash] = { token.line, token.column, words };
  }

  if( future.isCanceled() == true ) {
    future.reportCanceled();
    return;
  }

  /* Done, report the words that should be spellchecked */
  future.reportResult( ResultType{ std::move( newHashesOut ), std::move( newSettingsApplied ) } );
}
// --------------------------------------------------

QStringSet CppDocumentProcessor::getWordsThatAppearInSource() const
{
  QStringSet wordsSet;
  SP_CHECK( docPtr != nullptr );
  const uint32_t total = d->docPtr->globalSymbolCount();
  CPlusPlus::Overview overview;
  for( uint32_t i = 0; i < total; ++i ) {
    CPlusPlus::Symbol* symbol = d->docPtr->globalSymbolAt( i );
    wordsSet += getListOfWordsFromSourceRecursive( symbol, overview );
  }
  return wordsSet;
}
// --------------------------------------------------

QStringSet CppDocumentProcessor::getListOfWordsFromSourceRecursive( const CPlusPlus::Symbol* symbol, const CPlusPlus::Overview& overview ) const
{
  QStringSet wordsInSource;
  /* Get the pretty name and type for the current symbol. This name is then split up into
   * different words that are added to the list of words that appear in the source */
  const QString name = overview.prettyName( symbol->name() ).trimmed();
  const QString type = overview.prettyType( symbol->type() ).trimmed();
  wordsInSource += getPossibleNamesFromString( name );
  wordsInSource += getPossibleNamesFromString( type );

  /* Go to the next level into the scope of the symbol and get the words from that level as well*/
  const CPlusPlus::Scope* scope = symbol->asScope();
  if( scope != nullptr ) {
    CPlusPlus::Scope::iterator cur = scope->memberBegin();
    while( cur != scope->memberEnd() ) {
      const CPlusPlus::Symbol* curSymbol = *cur;
      ++cur;
      if( !curSymbol ) {
        continue;
      }
      wordsInSource += getListOfWordsFromSourceRecursive( curSymbol, overview );
    }
  }
  return wordsInSource;
}
// --------------------------------------------------

QStringSet CppDocumentProcessor::getPossibleNamesFromString( const QString& string ) const
{
  QStringSet wordSet;
  static const QRegularExpression nameRegExp( QStringLiteral( "(\\w+)" ) );
  QRegularExpressionMatchIterator i = nameRegExp.globalMatch( string );
  while( i.hasNext() ) {
    const QRegularExpressionMatch match = i.next();
    wordSet += match.captured( 1 );
  }
  return wordSet;
}
// --------------------------------------------------

WordTokens CppDocumentProcessor::parseToken( const CPlusPlus::Token& token, WordTokens::Type type ) const
{
  int32_t line;
  int32_t col;
  /* Get the index of the token. The index is used to get the position of the token.
   * Doing this first so that the token can be ignored if it is the first comment */
  const int32_t tokenBegin = token.utf16charsBegin();
  d->trUnit->getPosition( tokenBegin, &line, &col );
  /* Check if the first comment should be be returned.
   * This will be checked for every token, including literals and doxygen
   * comments. The check relies on early return of the if, thus the options
   * that are most likely to result in the check failing is first.
   * Doxygen comments are ignored since they normally are not used for headers
   * and if they are they might contain more than just the license. */
  if( ( type == WordTokens::Type::Comment )
      && ( line == 1 )
      && ( col == 1 )
      && ( d->settings.removeFirstComment == true ) ) {
    return {};
  }
  /* Get the token string */
  const QString tokenString = QString::fromUtf8( d->docPtr->utf8Source().mid( token.bytesBegin(), token.bytes() ).trimmed() );
  /* Calculate the hash of the token string */
  const uint32_t hash = qHash( tokenString );

  /* Set up the known parts of the return structure.
   * The rest will be populated as needed below. */
  WordTokens tokens;
  tokens.hash   = hash;
  tokens.column = col;
  tokens.line   = line;
  tokens.string = tokenString;
  tokens.type   = type;

  TmpOptional wordOpt = checkHash( tokens, hash );
  if( wordOpt.first == true ) {
    return wordOpt.second;
  }

  /* Token was not in the list of hashes.
   * Tokenize the string to extract words that should be checked. */
  tokens.words   = extractWordsFromString( tokenString, tokenBegin, type );
  tokens.newHash = true;
  return tokens;
}
// --------------------------------------------------

WordList CppDocumentProcessor::extractWordsFromString( const QString& string, int32_t stringStart, WordTokens::Type type ) const
{
  WordList wordTokens;
  const int32_t strLength = string.length();
  bool busyWithWord       = false;
  int32_t wordStartPos    = 0;
  bool endOfWord          = false;

  /* Iterate through all of the characters in the comment and extract words from them.
   * Words are split up by non-word characters and is checked using the isEndOfCurrentWord()
   * function. The end condition is deliberately set to continue past the length of the
   * comment so that a word in progress is stopped and extracted when the comment ends */
  for( int currentPos = 0; currentPos <= strLength; ++currentPos ) {
    /* Check if the current character is the end of a word character. */
    endOfWord = isEndOfCurrentWord( string, currentPos );
    if( ( endOfWord == false ) && ( busyWithWord == false ) ) {
      wordStartPos = currentPos;
      busyWithWord = true;
    }

    if( ( busyWithWord == true ) && ( endOfWord == true ) ) {
      /* Pre-condition sanity checks for debugging. The wordStartPos
       * can not be 0 or negative. A comment or literal always starts
       * with either a slash-star or slash-slash (comment) or inverted
       * comma (literal), thus there must always be something else before
       * the word starts.
       *
       * currentPos on the other hand can be at the end of the string
       * for example with a single line comment (slash-slash).
       */
      SP_CHECK( wordStartPos > 0 );
      Word word;
      word.fileName  = d->fileName;
      word.text      = string.mid( wordStartPos, currentPos - wordStartPos );
      word.start     = wordStartPos;
      word.end       = currentPos;
      word.length    = currentPos - wordStartPos;
      word.charAfter = ( currentPos < strLength )
                       ? string.at( currentPos )
                       : QLatin1Char( ' ' );
      word.inComment = ( type != WordTokens::Type::Literal );
      bool isDoxygenTag = false;
      if( type == WordTokens::Type::Doxygen ) {
        const QChar charBeforeStart = string.at( wordStartPos - 1 );
        if( ( charBeforeStart == QLatin1Char( '\\' ) )
            || ( charBeforeStart == QLatin1Char( '@' ) ) ) {
          const QString& currentWord = word.text;
          /* Classify it */
          const int32_t doxyClass = CppTools::classifyDoxygenTag( currentWord.unicode(), currentWord.size() );
          if( doxyClass != CppTools::T_DOXY_IDENTIFIER ) {
            /* It is a doxygen tag, mark it as such so that it does not end up
             * in the list of words from this string. */
            isDoxygenTag = true;
          }
        }
      }
      if( isDoxygenTag == false ) {
        d->trUnit->getPosition( stringStart + uint32_t( wordStartPos ), &word.lineNumber, &word.columnNumber );
        wordTokens.append( std::move( word ) );
      }
      busyWithWord = false;
      wordStartPos = 0;
    }
  }
  return wordTokens;
}
// --------------------------------------------------

bool CppDocumentProcessor::isEndOfCurrentWord( const QString& comment, int currentPos ) const
{
  /* Check to see if the current position is past the length of the comment. If this
   * is the case, then clearly it is the end of the current word */
  if( currentPos >= comment.length() ) {
    return true;
  }

  const QChar& currentChar = comment[currentPos];

  /* Check if the current character is a letter, number or underscore.
   * Some settings might change what the end of a word actually is.
   * For some settings an underscore will be considered as the end of a word */
  if( ( currentChar.isLetterOrNumber() == true )
      || ( currentChar == QLatin1Char( '_' ) ) ) {
    return false;
  }

  /* Check for an apostrophe in a word. This is for words like we're. Not all
   * apostrophes are part of a word, like words that starts with and end with */
  if( currentChar == QLatin1Char( '\'' ) ) {
    /* Do some range checking, is this is the first or last character then
     * this is not part of a word, thus it is the end of the current word */
    if( ( currentPos == 0 ) || ( currentPos == ( comment.length() - 1 ) ) ) {
      return true;
    }
    if( ( comment.at( currentPos + 1 ).isLetter() == true )
        && ( comment.at( currentPos - 1 ).isLetter() == true ) ) {
      return false;
    }
  }

  /* For words with '.' in, such as abbreviations and email addresses */
  if( currentChar == QLatin1Char( '.' ) ) {
    if( ( currentPos == 0 ) || ( currentPos == ( comment.length() - 1 ) ) ) {
      return true;
    }
    static const QRegularExpression wordChars( QStringLiteral( "\\w" ) );
    if( ( wordChars.match( comment.at( currentPos + 1 ) ).hasMatch() )
        && ( wordChars.match( comment.at( currentPos - 1 ) ).hasMatch() ) ) {
      return false;
    }
  }

  /* For word with @ in: Email address */
  if( currentChar == QLatin1Char( '@' ) ) {
    if( ( currentPos == 0 ) || ( currentPos == ( comment.length() - 1 ) ) ) {
      return true;
    }
    static const QRegularExpression wordChars( QStringLiteral( "\\w" ) );
    if( ( wordChars.match( comment.at( currentPos + 1 ) ).hasMatch() )
        && ( wordChars.match( comment.at( currentPos - 1 ) ).hasMatch() ) ) {
      return false;
    }
  }

  /* Check for websites. This will only check the website characters if the
   * option for website addresses are enabled due to the amount of false positives
   * that this setting can remove. Also this can put some overhead to other settings
   * that are not always desired.
   * This setting might require some rework in the future. */
  if( d->settings.removeWebsites == true ) {
    static const QRegularExpression websiteChars( QStringLiteral( "\\w|" ) + QLatin1String( Parsers::CppParser::Constants::WEBSITE_CHARS_REGEXP_PATTERN ) );
    if( websiteChars.match( currentChar ).hasMatch() == true ) {
      if( ( currentPos == 0 ) || ( currentPos == ( comment.length() - 1 ) ) ) {
        return true;
      }
      if( ( websiteChars.match( comment.at( currentPos + 1 ) ).hasMatch() == true )
          && ( websiteChars.match( comment.at( currentPos - 1 ) ).hasMatch() == true ) ) {
        return false;
      }
    }
  }

  return true;
}
// --------------------------------------------------

QVector<WordTokens> CppDocumentProcessor::parseMacros() const
{
  /* Get the macros from the document pointer. The arguments of the macro will then be parsed
   * and checked for spelling mistakes.
   * Since the TranslationUnit::getPosition() is not usable with the Macro Uses a manual method to
   * extract the literals and their line and column positions are implemented. This involves getting
   * the unprocessed source from the document for the macro and tracking the byte offsets manually
   * from there. An alternative implementation can involve using the
   * Snapshot::preprocessedDocument(), which makes use of the FastPreprocessor and the AST visitors.
   * For more information around a discussion on the mailing list around this issue refer to the
   * following
   * link: http://comments.gmane.org/gmane.comp.lib.qt.creator/11853 */
  QVector<WordTokens> tokenizedWords;
  QList<CPlusPlus::Document::MacroUse> macroUse = d->docPtr->macroUses();
  if( macroUse.count() == 0 ) {
    return {};
  }
  static CppTools::CppModelManager* cppModelManager    = CppTools::CppModelManager::instance();
  CppTools::CppEditorDocumentHandle* cppEditorDocument = cppModelManager->cppEditorDocument( d->docPtr->fileName() );
  if( cppEditorDocument == nullptr ) {
    return {};
  }
  const QByteArray source = cppEditorDocument->contents();

  for( const CPlusPlus::Document::MacroUse& mac: macroUse ) {
    if( mac.isFunctionLike() == false ) {
      /* Only macros that are function like should be considered.
       * These are ones that have arguments, that can be literals.
       */
      continue;
    }
    const QVector<CPlusPlus::Document::Block>& args = mac.arguments();
    if( args.count() == 0 ) {
      /* There are no arguments for the macro, no need to consider
       * further. */
      continue;
    }
    /* The line number of the macro is used as the reference. */
    uint32_t line = mac.beginLine();
    /* Get the start of the line from the source. From this start the offset to the
     * words will be calculated. This start should never be -1 since
     * a line will always start with a newline, and it seems like a macro
     * will never start on the first line of the source file.
     * Added a debug assert just to ensure this. */
    const uint32_t start = uint32_t( source.lastIndexOf( '\n', int32_t( mac.utf16charsBegin() ) ) );
    SP_CHECK( source.lastIndexOf( '\n', int32_t( mac.utf16charsBegin() ) ) > 0 );
    /* Get the end index of the last argument of the macro. */
    const uint32_t end = args.last().utf16charsEnd();
    SP_CHECK( start < end );
    if( end == 0 ) {
      /* This will happen on an empty argument, for example Q_ASSERT() */
      continue;
    }

    /* Get the full macro from the start of the line until the last byte of the last
     * argument. */
    const QByteArray macroBytes = source.mid( int32_t( start ), int32_t( end - start ) );
    /* Quick check to see if there are any string literals in the macro text.
     * if the are this check can be a waist, but if not this can speed up the check by
     * avoiding an unneeded regular expression. */
    if( macroBytes.contains( '\"' ) == false ) {
      continue;
    }

    /* Check if the hash of the macro is not already contained
     * in the list of known hashes. The hash is calculated from the
     * start of the macro and not the line so that the movement of the
     * macro due to edits before the macro can be handled using the hash
     * functionality.*/
    WordTokens tokens;
    tokens.column  = mac.utf16charsBegin() - start;
    tokens.line    = line;
    tokens.string  = QString::fromUtf8( macroBytes.mid( int32_t( mac.utf16charsBegin() - start ) ) );
    tokens.hash    = qHash( tokens.string );
    tokens.type    = WordTokens::Type::Literal;
    tokens.newHash = true;

    TmpOptional wordOpt = checkHash( tokens, tokens.hash );
    if( wordOpt.first == true ) {
      tokenizedWords.append( wordOpt.second );
      continue;
    }

    /* Get the byte offsets inside the macro bytes for each line break inside the macro.
     * This will be used during the extraction to get the correct lines relative to the
     * line that the macro started on. */
    QVector<uint32_t> lineIndexes;
    /* The search above for the start include the new line character so the start for other
     * new lines must start from index 1 so that it is not also included. */
    int32_t startSearch = 1;
    while( true ) {
      int idx = macroBytes.indexOf( '\n', startSearch );
      if( idx < 0 ) {
        /* No more new lines, break out */
        break;
      }
      lineIndexes << uint32_t( idx );
      /* Must increment the start of the next search otherwise it will be found on
       * that index. */
      startSearch = idx + 1;
    }
    /* The end is also added to simplify some of the checks further on. No character can
     * fall outside of the end. */
    lineIndexes << uint32_t( end );
    uint32_t lineBreak = lineIndexes.takeFirst();
    uint32_t colOffset = 0;

    /* Use a regular expression to get all string literals from the macro and its arguments. */
    static const QRegularExpression regExp( QStringLiteral( "\"([^\"\\\\]|\\\\.)*\"" ) );
    QRegularExpressionMatchIterator regExpIter = regExp.globalMatch( QString::fromLatin1( macroBytes ) );
    while( regExpIter.hasNext() == true ) {
      const QRegularExpressionMatch match = regExpIter.next();
      const QString tokenString           = match.captured( 0 );
      SP_CHECK( match.capturedStart( 0 ) >= 0 );
      const uint32_t capStart = uint32_t( match.capturedStart( 0 ) );
      /* Check if the literal starts on the next line from the current one */
      while( capStart > lineBreak ) {
        /* Increase the line number and reset the column offset */
        ++line;
        colOffset = lineBreak;
        lineBreak = lineIndexes.takeFirst();
      }
      /* Get the words from the extracted literal */
      WordList words = extractWordsFromString( tokenString, 0, WordTokens::Type::Literal );
      for( Word& word: words ) {
        /* Apply the offsets to the words */
        word.columnNumber += capStart - colOffset;
        word.lineNumber    = line;
      }
      tokens.words.append( words );
      /* Get the words from the extracted literal */
    }
    if( tokens.words.count() != 0 ) {
      tokenizedWords.append( tokens );
    }
  }
  return tokenizedWords;
}
// --------------------------------------------------

CppDocumentProcessor::TmpOptional CppDocumentProcessor::checkHash( WordTokens tokens, uint32_t hash ) const
{
  /* Search if the hash contains the given token. If it does
   * then the words that got extracted previously are used
   * as is, without attempting to extract them again. If the
   * token is not in the hash, it is a new token and must be
   * parsed to get the words from the token. */
  HashWords::const_iterator iter          = d->tokenHashes.constFind( hash );
  const HashWords::const_iterator iterEnd = d->tokenHashes.constEnd();
  if( iter != iterEnd ) {
    /* The token was parsed in a previous iteration.
     * Now check if the token moved due to lines being
     * added or removed. It it did not move, use the
     * words as is, if it did move, adjust the line and
     * column number of the words by the amount that the
     * token moved. */
    const TokenWords& tokenWords = ( iter.value() );
    if( ( tokenWords.line == tokens.line )
        && ( tokenWords.col == tokens.column ) ) {
      tokens.words   = tokenWords.words;
      tokens.newHash = false;
      return std::make_pair( true, tokens );
    } else {
      WordList words;
      /* Token moved, adjust.
       * This will even work for lines that are copied because the
       * hash will be the same but the start will just be different. */
      const qint32 lineDiff    = int32_t( tokenWords.line ) - int32_t( tokens.line );
      const qint32 colDiff     = int32_t( tokenWords.col ) - int32_t( tokens.column );
      const uint32_t firstLine = tokens.line;
      /* Move the line according to the difference between the
       * known position and the position from the hash.
       * The column is also moved, but only if on the first line
       * since a column move is only possible on the first line.
       * If a column moved that are not on the first line, the hash
       * would be new and it would be regarded as a new hash. A move
       * on the column will not cause this, but will also not move the
       * words below it, thus they should not be updated. */
      for( Word word: qAsConst( tokenWords.words ) ) {
        word.lineNumber = uint32_t( int32_t( word.lineNumber ) - lineDiff );
        if( word.lineNumber == firstLine ) {
          word.columnNumber = uint32_t( int32_t( word.columnNumber ) - colDiff );
        }
        words.append( word );
      }
      tokens.words   = words;
      tokens.newHash = false;
      return std::make_pair( true, tokens );
    }
  }
  return std::make_pair( false, WordTokens{} );
}
// --------------------------------------------------
