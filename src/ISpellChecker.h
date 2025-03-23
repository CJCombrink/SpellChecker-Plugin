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

#pragma once

#include "Word.h"

#include <QFuture>
#include <QObject>
#include <QSettings>

namespace SpellChecker {

class IOptionsWidget;

/*! \brief The ISpellChecker Interface
 *
 * Interface for spellchecker implementations.
 */
class ISpellChecker
  : public QObject
{
  Q_OBJECT
public:
  ISpellChecker() {}
  virtual ~ISpellChecker() {}

  /*! \brief Get the name of the Spell Checker.
   * \return String name of the spell checker.
   */
  virtual QString name() const = 0;
  /*! \brief Query if a given word is is a spelling mistake
   * \param[in] word Word that must be checked.
   * \return True if the word is a spelling mistake.
   */
  virtual bool isSpellingMistake( const QString& word ) const = 0;
  /*! \brief Get suggestions for a given word.
   * \param[in] word Misspelled word that suggestions for correct spellings
   *                  are required.
   * \param[out] suggestions List of suggested correct spellings for the
   *                  give \a word.
   */
  virtual void getSuggestionsForWord( const QString& word, QStringList& suggestions ) const = 0;
  /*! \brief Add a word to the user's dictionary.
   * \param[in] word Word to add to the dictionary.
   * \returns True if the word was added.
   */
  virtual bool addWord( const QString& word ) = 0;
  /*! \brief Ignore a word.
   *
   * Ignoring a word is only temporary for the current session.
   * The word will be marked as a spelling mistake after Qt Creator
   * is closed and opened again.
   *
   * A more permanent solution to ignoring the word is to
   * add the word to the user's personal dictionary using the
   * addWord() function.
   * \param[in] word Word to ignore.
   * \returns True if the words was ignored.
   */
  virtual bool ignoreWord( const QString& word ) = 0;
  /*! \brief Get the options widget for the Spell Checker implementation.
   *
   * The caller of this function will become the owner of the returned
   * widget and will delete it as needed. Thus this function should
   * return a new widget each time it is called.
   * \return Pointer to the options widget.
   */
  virtual IOptionsWidget* optionsWidget() = 0;
};

/*! \brief The SpellCheckProcessor class
 *
 * This processor class is used by a QtConcurrent process to use the
 * SpellChecker interface to spell check words extracted from the files
 * in a separate thread in the background. This is done to release the
 * document parser as soon as possible and spell checking blocks the main
 * thread if done there.
 *
 * The processor uses the spell checker that is set on the application
 * to check the words that were extracted from the file and check them
 * for spelling mistakes. To speed up the checking the processor also takes
 * in the list of previous mistakes extracted from the file. This is done
 * since the suggestions for misspelled words can be reused without having to
 * ask the spell checker for suggestions each time for repeating words.
 * The chance that a word repeats in a file is big for different passes and the
 * time for a spell checker to get suggestions can be rather slow.
 *
 * This process can be cancelled by cancelling the future. */
class SpellCheckProcessor
  : public QObject
{
  Q_OBJECT
public:
  /*! \brief Create the SpellCheckProcessor with a spell checker, for a file and the
   * given list of words.
   *
   * \param[in] spellChecker Spell Checker object that must be used. This spell checker
   *      must be thread safe.
   * \param[in] fileName Name of the file that the given words to be checked belongs to.
   * \param[in] wordList Words that must be checked for possible spelling mistakes.
   * \param[in] previousMistakes List of words that were identified as spelling mistakes in
   *      the previous processing run of the current file.*/
  SpellCheckProcessor( ISpellChecker* spellChecker, const QString& fileName, const WordList& wordList, const WordList& previousMistakes );
  ~SpellCheckProcessor();
  /*! Function that will run in the background/thread. */
  void process(QPromise<WordList>& promise );
protected:
  ISpellChecker* d_spellChecker;
  QString  d_fileName;
  WordList d_wordList;
  WordList d_previousMistakes;
};

} // namespace SpellChecker
