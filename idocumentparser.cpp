/**************************************************************************
**
** Copyright (c) 2014 Carel Combrink
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

#include "idocumentparser.h"

using namespace SpellChecker;

IDocumentParser::IDocumentParser( QObject* parent )
  : QObject( parent )
{}
// --------------------------------------------------

IDocumentParser::~IDocumentParser()
{}
// --------------------------------------------------

bool IDocumentParser::isReservedWord( const QString& word )
{
  /* Trying to optimize the check using the same method as used
   * in the cpptoolsreuse.cpp file in the CppTools plugin. */
  switch( word.length() ) {
    case 3:
      switch( word.at( 0 ).toUpper().toLatin1() ) {
        case 'C':
          if( word.toUpper() == QStringLiteral( "CPP" ) ) {
            return true;
          }
          break;
        case 'S':
          if( word.toUpper() == QStringLiteral( "STD" ) ) {
            return true;
          }
          break;
      }
      break;
    case 4:
      switch( word.at( 0 ).toUpper().toLatin1() ) {
        case 'E':
          if( word.toUpper() == QStringLiteral( "ENUM" ) ) {
            return true;
          }
          break;
      }
      break;
    case 6:
      switch( word.at( 0 ).toUpper().toLatin1() ) {
        case 'S':
          if( word.toUpper() == QStringLiteral( "STRUCT" ) ) {
            return true;
          }
          break;
        case 'P':
          if( word.toUpper() == QStringLiteral( "PLUGIN" ) ) {
            return true;
          }
          break;
      }
      break;
    case 7:
      switch( word.at( 0 ).toUpper().toLatin1() ) {
        case 'D':
          if( word.toUpper() == QStringLiteral( "DOXYGEN" ) ) {
            return true;
          }
          break;
        case 'N':
          if( word.toUpper() == QStringLiteral( "NULLPTR" ) ) {
            return true;
          }
          break;
        case 'T':
          if( word.toUpper() == QStringLiteral( "TYPEDEF" ) ) {
            return true;
          }
          break;
      }
      break;
    case 9:
      switch( word.at( 0 ).toUpper().toLatin1() ) {
        case 'N':
          if( word.toUpper() == QStringLiteral( "NAMESPACE" ) ) {
            return true;
          }
          break;
      }
      break;
    default:
      break;
  }
  return false;
}
// --------------------------------------------------

void IDocumentParser::getWordsFromSplitString( const QStringList& stringList, const Word& word, WordList& wordList )
{
  /* Now that the words are split, they need to be added to the WordList correctly */
  int numbSplitWords = stringList.count();
  int currentPos     = 0;
  for( int wordIdx = 0; wordIdx < numbSplitWords; ++wordIdx ) {
    Word newWord;
    newWord.text         = stringList.at( wordIdx );
    newWord.fileName     = word.fileName;
    currentPos           = ( word.text ).indexOf( newWord.text, currentPos );
    newWord.columnNumber = word.columnNumber + currentPos;
    newWord.lineNumber   = word.lineNumber;
    newWord.length       = newWord.text.length();
    newWord.start        = word.start + currentPos;
    newWord.end          = newWord.start + newWord.length;
    newWord.inComment    = word.inComment;
    currentPos           = currentPos + newWord.length;
    /* Add the word to the end of the word list so that it can be checked against the
     * settings later on */
    wordList.append( newWord );
  }
}
// --------------------------------------------------

void IDocumentParser::removeWordsThatAppearInSource( const QStringSet& wordsInSource, WordList& words )
{
  /* Hopefully all words that are the same would be together in the WordList because of
   * the nature of the QMultiHash. This would make removing multiple instances of the same
   * word faster.
   *
   * The idea is to save the last word that was removed, and then for the next word,
   * first compare it to the last word removed, and if it is the same, remove it from the list. Else
   * search for the word in the wordsInSource list. The reasoning is that the first compare will be
   * faster than the search in the wordsInSource list. But with this the overhead is added that now
   * there is perhaps one more compare for each word that is not present in the source. Perhaps this
   * can then be slower because the probability is that there will be more words not in the source
   * than there are duplicate words that are in the source. For now this will be left this way but
   * more benchmarking needs to be done to optimize.
   *
   * An initial test using QTime::start() and QTime::elapsed() showed the same speed of 3ms for
   * either option for about 1425 potential words checked in 98 words occurring in the source. 148
   * Words were removed for this test case. NOTE: Due to the inaccuracy of the timer on windows a
   * better test will be performed in future. */
#ifdef USE_MULTI_HASH
  WordList::Iterator iter = words.begin();
  QString lastWordRemoved;
  while( iter != words.end() ) {
    if( iter.key() == lastWordRemoved ) {
      iter = words.erase( iter );
    } else if( wordsInSource.contains( iter.key() ) ) {
      lastWordRemoved = iter.key();
      /* The word does appear in the source, thus remove it from the list of
       * potential words that must be checked */
      iter = words.erase( iter );
    } else {
      ++iter;
    }
  }
#else /* USE_MULTI_HASH */
  WordList::Iterator iter = words.begin();
  while( iter != words.end() ) {
    if( wordsInSource.contains( ( *iter ).text ) ) {
      /* The word does appear in the source, thus remove it from the list of
       * potential words that must be checked */
      iter = words.erase( iter );
    } else {
      ++iter;
    }
  }
#endif /* USE_MULTI_HASH */
}
// --------------------------------------------------
