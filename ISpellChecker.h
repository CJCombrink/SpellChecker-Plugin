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

#ifndef ISPELLCHECKER_H
#define ISPELLCHECKER_H

#include "Word.h"

#include <QSettings>

namespace SpellChecker {

/*! \brief The ISpellChecker Interface
 *
 * Interface for spellchecker implementations.
 */
class ISpellChecker {
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
    virtual bool isSpellingMistake(const QString& word) const = 0;
    /*! \brief Get suggestions for a given word.
     * \param[in] word Misspelled word that suggestions for correct spellings
     *                  are required.
     * \param[out] suggestions List of suggested correct spellings for the
     *                  give \a word.
     */
    virtual void getSuggestionsForWord(const QString& word, QStringList& suggestions) const = 0;
    /*! \brief Add a word to the user's dictionary.
     * \param[in] word Word to add to the dictionary.
     */
    virtual void addWord(const QString& word) = 0;
    /*! \brief Ignore a word.
     *
     * Ignoring a word is only temporary for the current session.
     * The word will be marked as a spelling mistake after Qt Creator
     * is closed and opened again.
     *
     * A more permanent solution to ignoring the word is to
     * add the word to the user's personal dictionary using the
     * addWord() function/
     * \param[in] word Word to ignore.
     */
    virtual void ignoreWord(const QString& word) = 0;
    /*! \brief Get the options widget for the Spell Checker implementation.
     * \return Pointer to the options widget.
     */
    virtual QWidget* optionsWidget() = 0;
};

}

#endif // ISPELLCHECKER_H