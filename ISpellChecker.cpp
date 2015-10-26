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

using namespace SpellChecker;

SpellCheckProcessor::SpellCheckProcessor(ISpellChecker *spellChecker, const QString &fileName, const WordList &wordList)
    : d_spellChecker(spellChecker)
    , d_fileName(fileName)
    , d_wordList(wordList)
{
}
//--------------------------------------------------

SpellCheckProcessor::~SpellCheckProcessor()
{
}
//--------------------------------------------------

void SpellCheckProcessor::process(QFutureInterface<WordList> &future)
{
    WordListConstIter misspelledIter;
    Word misspelledWord;
    WordList misspelledWords;
    WordList words = d_wordList;
    WordListConstIter wordIter = words.constBegin();
    bool spellingMistake;
    future.setProgressRange(0, words.count());
    while(wordIter != d_wordList.constEnd()) {
        misspelledWord = (*wordIter);
        if(future.isCanceled() == true) {
            return;
        }
        /* Search for the word in the list of words that were already
         * identified as spelling mistakes. If there are words that are
         * repeated and misspelled, this can reduce the time to process
         * the file since the time to get suggestions is rather slow.
         * If there are no repeating mistakes then this might add unneeded
         * overhead. */
        misspelledIter = misspelledWords.find(misspelledWord.text);
        if(misspelledIter != misspelledWords.constEnd()) {
            misspelledWord.suggestions = (*misspelledIter).suggestions;
            /* Add the word to the local list of misspelled words. */
            misspelledWords.append(misspelledWord);
        } else {
            spellingMistake = d_spellChecker->isSpellingMistake(misspelledWord.text);
            /* Check to see if the char after the word is a period. If it is,
             * add the period to the word an see if it passes the checker. */
            if((spellingMistake == true)
                    && ((*wordIter).charAfter == QLatin1Char('.'))) {
                /* Recheck the word with the period added */
                spellingMistake = d_spellChecker->isSpellingMistake(misspelledWord.text + QLatin1Char('.'));
            }

            if(spellingMistake == true) {
                d_spellChecker->getSuggestionsForWord(misspelledWord.text, misspelledWord.suggestions);
                /* Add the word to the local list of misspelled words. */
                misspelledWords.append(misspelledWord);
            }
        }
        ++wordIter;
        future.setProgressValue(future.progressValue() + 1);
    }
    future.reportResult(misspelledWords);
}
//--------------------------------------------------
