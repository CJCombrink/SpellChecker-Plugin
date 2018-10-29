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

#include <projectexplorer/project.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QObject>
#include <QSettings>

namespace Core { class IOptionsPage; }

namespace SpellChecker {
namespace Internal {
class SpellCheckerCorePrivate;
class OutputPane;
class SpellCheckerCoreSettings;
class ProjectMistakesModel;
}
class IDocumentParser;
class ISpellChecker;

/*!
 * \brief The SpellCheckerCore class
 *
 * This is the core class of the spell checker. It handles document parser, spell
 * checkers and events in the application.
 *
 * Users can add different document parsers as well as spell checkers to the
 * core by calling the correct set functions.
 */
class SpellCheckerCore : public QObject
{
    Q_OBJECT
public:
    SpellCheckerCore(QObject *parent = 0);
    ~SpellCheckerCore();

    static SpellCheckerCore* instance();
    /*!
     * \brief Add a new document parser to the Spell Checker.
     * \param parser New parser that can also be used to parse documents.
     * \return False if the parser could not be added. This would normally
     *          happen when the parser was added to the core already.
     */
    bool addDocumentParser(IDocumentParser* parser);
    /*!
     * \brief Remove a document parser previously added to the Spell Checker.
     * \param parser Parser that should be removed.
     */
    void removeDocumentParser(IDocumentParser* parser);


    Internal::OutputPane* outputPane() const;

    /*! \brief Get the current active spell checker that will be used to check spelling.
     *
     * If there were no spell checkers added using the add or set functions, this
     * function will return an empty pointer. */
    ISpellChecker* spellChecker() const;
    /*! \brief Get the list of spell checkers added to this object. */
    QMap<QString, ISpellChecker*> addedSpellCheckers() const;
    /*! \brief Add a spell checker to the list of available checkers.
     *
     * If there are no active spell checker set on the object, the added
     * object will be set as the active checker.
     * \sa setSpellChecker()
     * \sa spellChecker() */
    void addSpellChecker(ISpellChecker* spellChecker);
    /*! \brief Set the supplied spell checker object as the current active
     * spell checker.
     *
     * The given checker will be added to the list of checkers if it
     * is not already added.
     * \sa addSpellChecker()
     * \sa spellChecker() */
    void setSpellChecker(ISpellChecker* spellChecker);

    Core::IOptionsPage* optionsPage();
    /*! \brief Get the Core Settings. */
    Internal::SpellCheckerCoreSettings* settings() const;
    Internal::ProjectMistakesModel* spellingMistakesModel() const;

    /*! \brief Is the Word Under the Cursor a Mistake
     * Check if the word under the cursor is a spelling mistake, and if it is,
     * return the misspelled word.
     * \param[out] word If the word is a mistake, this will return the misspelled word.
     * \return True if the word is misspelled.
     */
    bool isWordUnderCursorMistake(Word& word) const;
    /*! \brief Replace Words In CurrentEditor.
     * Replace the given words in the current editor with the supplied replacement word.
     * \param[in] wordsToReplace List of words to replace
     * \param[in] replacementWord Word to replace all occurrences of the \a wordsToReplace
     *              with.
     */
    void replaceWordsInCurrentEditor(const WordList& wordsToReplace, const QString& replacementWord);

private:
    enum RemoveAction {
        None = 0, /*!< No Action should be taken. */
        Ignore,   /*!< Ignore the word for the current Qt Creator Session. */
        Add       /*!< Add the word to the user's custom dictionary. */
    };
    /*! \brief Get All Occurrences Of a Word.
     * \param[in] word Word that must be retrieved.
     * \param[out] List of words that are the same as the \a word.
     * \return true if at least one occurrence was found.
     */
    bool getAllOccurrencesOfWord(const Word &word, WordList& words);
    /*! \brief Remove Word Under Cursor.
     * Remove the word under the cursor using the remove action.
     * \param[in] action Action to use to remove the word.
     */
    void removeWordUnderCursor(RemoveAction action);

signals:
    /*! \brief Signal emitted to inform the plugin if the word under the cursor is a mistake.
     *
     * This signal gets emitted in response to the cursorPositionChanged() slot getting
     * called if the cursor position changes.
     * \param isMistake True if the word under the cursor is a mistake.
     * \param word The misspelled word if the word under the cursor is a mistake. */
    void wordUnderCursorMistake(bool isMistake, const SpellChecker::Word& word = SpellChecker::Word());
    /*! \brief Signal emitted by the core to notify the parsers that the editor changed.
     *
     * This signal gets emitted in response to the Qt Creator framework invoking the
     * currentEditorChanged(Core::IEditor *editor) slot on the core if the editor is
     * changed.
     *
     * The signal gets the name of the editor and then communicate the name with the
     * parsers. There should be no need for the parsers to have a pointer to the editor.
     * \param filePath The file path/ name of the current editor. */
    void currentEditorChanged(const QString& filePath);
    /*! \brief Signal emitted by the core if the active project changes.
     *
     * This signal gets emitted in response to the Qt Creator framework invoking the
     * startupProjectChanged() slot to notify parsers that the active project changed.
     * \param startupProject Pointer to the startup project. */
    void activeProjectChanged(ProjectExplorer::Project *startupProject);

public slots:
    /*! \brief Open the suggestions widget for the word under the cursor. */
    void giveSuggestionsForWordUnderCursor();
    /*! \brief Ignore the word under the cursor for this instance of the Qt Creator application. */
    void ignoreWordUnderCursor();
    /*! \brief Add the word under the cursor to the user dictionary. */
    void addWordUnderCursor();
    /*! \brief Feeling Lucky Option.
     *
     * Replaces the misspelled word with the first suggestion for the word.
     * This is very useful when there is only one suggestion for the word
     * and it is the correct spelling for the misspelled word.*/
    void replaceWordUnderCursorFirstSuggestion();
    /*! \brief Add the misspelled words to the list of mistakes.
     *
     * This function gets called to add word to the list of misspelled word.
     * As a slot this function gets called by the spellchecker when finished
     * checking all words for the given file.
     * \param[in] fileName Name of the file that the misspelled words belong to.
     * \param[in] words List of misspelled words for the given file. */
    void addMisspelledWords(const QString& fileName, const SpellChecker::WordList& words);

private slots:
    /*! \brief Spellcheck Words from Parser
     * Spell check words parsed by the parser of the given file. This
     * will relay the words to the set spellchecker that will then spell
     * check the words and give suggestions for misspelled words.
     *
     * \param[in] fileName Name of the file that the words belong to.
     * \param[in] words List of words that must be checked for spelling mistakes.
     */
    void spellcheckWordsFromParser(const QString &fileName, const SpellChecker::WordList& words);
    /*! \brief Slot called when the Qt Creator Startup or active project changes. */
    void startupProjectChanged(ProjectExplorer::Project* startupProject);
    /*! \brief Slot called when the files in the project changes. */
    void projectsFilesChanged();
    /*! \brief Slot called when the cursor position for the current editor changes.
     *
     * If the cursor is over a misspelled word, then the controls and actions for
     * spelling mistakes should become active so that the user can interact with the
     * misspelled words. */
    void cursorPositionChanged();
    /*! \brief Slot called when the current editor changes on the editor manager. */
    void mangerEditorChanged(Core::IEditor* editor);
    /*! \brief Slot called when an editor is opened. */
    void editorOpened(Core::IEditor* editor);
    /*! \brief Slot called when an editor is closed. */
    void editorAboutToClose(Core::IEditor* editor);
    /*! \brief Slot that will get called when the "Spell Check" right-click
     * Context menu is about to be shown.
     *
     * This slot updates the context menu with the possible spellings for the misspelled
     * words as actions that can be used to replace the misspelled words with the given
     * actions. */
    void updateContextMenu();
    /*! \brief Slot called when a Future is finished checking the spelling of potential
     * words. */
    void futureFinished();
    /*! \brief Slot called when the application quits to cancel all outstanding futures. */
    void cancelFutures();
    /*! \brief Slot called when Qt Creator is about to quit. */
    void aboutToQuit();
private:
    Internal::SpellCheckerCorePrivate* const d;
};
}
