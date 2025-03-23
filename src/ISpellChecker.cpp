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

#include "ISpellChecker.h"
#include "Word.h"

// #define BENCH_TIME
#ifdef BENCH_TIME
#include <QElapsedTimer>
#endif /* BENCH_TIME */

using namespace SpellChecker;

SpellCheckProcessor::SpellCheckProcessor( ISpellChecker* spellChecker, const QString& fileName, const WordList& wordList, const WordList& previousMistakes )
  : d_spellChecker( spellChecker )
  , d_fileName( fileName )
  , d_wordList( wordList )
  , d_previousMistakes( previousMistakes )
{}
// --------------------------------------------------

SpellCheckProcessor::~SpellCheckProcessor()
{}
// --------------------------------------------------

void SpellCheckProcessor::process( QPromise<WordList>& promise )
{
#ifdef BENCH_TIME
  QElapsedTimer timer;
  timer.start();
#endif /* BENCH_TIME */
  WordListConstIter misspelledIter;
  WordListConstIter prevMisspelledIter;
  Word misspelledWord;
  WordList misspelledWords;
  WordList words             = d_wordList;
  WordListConstIter wordIter = words.constBegin();
  bool spellingMistake;
  promise.setProgressRange( 0, words.count() + 1 );
  while( wordIter != d_wordList.constEnd() ) {
    /* Get the word at the current iterator position.
     * After this is done, move the iterator to the next position and
     * increment the progress value. This is done so that one can call
     * 'continue' anywhere after this in the loop without having to worry
     * about advancing the iterator since this will already be done and
     * correct for the next iteration. */
    misspelledWord = ( *wordIter );
    ++wordIter;
    promise.setProgressValue( promise.future().progressValue() + 1 );
    /* Check if the future was cancelled */
    if( promise.isCanceled() == true ) {
      return;
    }
    spellingMistake = d_spellChecker->isSpellingMistake( misspelledWord.text );
    /* Check to see if the char after the word is a period. If it is,
     * add the period to the word an see if it passes the checker. */
    if( ( spellingMistake == true )
        && ( misspelledWord.charAfter == QLatin1Char( '.' ) ) ) {
      /* Recheck the word with the period added */
      spellingMistake = d_spellChecker->isSpellingMistake( misspelledWord.text + QLatin1Char( '.' ) );
    }

    if( spellingMistake == true ) {
      /* The word is a spelling mistake, check if the word was a mistake
       * the previous time that this file was processed. If it was the
       * suggestions can be reused without having to get the suggestions
       * through the spell checker since this is slow compared to the rest
       * of the processing. */
      prevMisspelledIter = d_previousMistakes.constFind( misspelledWord.text );
      if( prevMisspelledIter != d_previousMistakes.constEnd() ) {
        misspelledWord.suggestions = ( *prevMisspelledIter ).suggestions;
        misspelledWords.append( misspelledWord );
        continue;
      }
      /* The word was not in the previous iteration, search for the word
       * in the list of words that were already checked in this iteration
       * and identified as spelling mistakes. If there are words that are
       * repeated and misspelled, this can reduce the time to process
       * the file since the time to get suggestions is rather slow.
       * If there are no repeating mistakes then this might add unneeded
       * overhead. */
      misspelledIter = misspelledWords.constFind( misspelledWord.text );
      if( misspelledIter != misspelledWords.constEnd() ) {
        misspelledWord.suggestions = ( *misspelledIter ).suggestions;
        /* Add the word to the local list of misspelled words. */
        misspelledWords.append( misspelledWord );
        continue;
      }

      /* Another checkpoint before we go into the SpellChecker to check for mistakes */
      if( promise.isCanceled() == true ) {
        return;
      }
      /* At this point the word is a mistake for the first time. It was neither
       * a mistake in the previous pass of the file nor did the word occur previously
       * in this file, use the spell checker to get the suggestions for the word. */
      d_spellChecker->getSuggestionsForWord( misspelledWord.text, misspelledWord.suggestions );
      /* Add the word to the local list of misspelled words. */
      misspelledWords.append( misspelledWord );
    }
  }
#ifdef BENCH_TIME
  qDebug() << "File: " << d_fileName
           << "\n  - time : " << timer.elapsed()
           << "\n  - count: " << misspelledWords.size();
#endif /* BENCH_TIME */

  if( promise.isCanceled() == true ) {
    return;
  }
  promise.addResult( misspelledWords );
}
// --------------------------------------------------
