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

#include "spellcheckercore.h"
#include "idocumentparser.h"
#include "ISpellChecker.h"
#include "outputpane.h"
#include "spellingmistakesmodel.h"
#include "spellcheckercoreoptionspage.h"
#include "spellcheckercoresettings.h"
#include "spellcheckerconstants.h"
#include "suggestionsdialog.h"
#include "NavigationWidget.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <texteditor/texteditor.h>
#include <cpptools/cppmodelmanager.h>

#include <QPointer>
#include <QMouseEvent>
#include <QTextCursor>
#include <QMenu>

class SpellChecker::Internal::SpellCheckerCorePrivate {
public:

    QList<QPointer<SpellChecker::IDocumentParser> > documentParsers;
    ProjectMistakesModel* spellingMistakesModel;
    SpellChecker::Internal::SpellingMistakesModel* mistakesModel;
    SpellChecker::Internal::OutputPane* outputPane;
    SpellChecker::Internal::SpellCheckerCoreSettings* settings;
    SpellChecker::Internal::SpellCheckerCoreOptionsPage* optionsPage;
    QMap<QString, ISpellChecker*> addedSpellCheckers;
    SpellChecker::ISpellChecker* spellChecker;
    QPointer<Core::IEditor> currentEditor;
    Core::ActionContainer *contextMenu;
    QString currentFilePath;
    ProjectExplorer::Project* startupProject;
    QStringList filesInStartupProject;

    SpellCheckerCorePrivate() :
        spellChecker(NULL),
        currentEditor(NULL),
        currentFilePath(),
        startupProject(NULL),
        filesInStartupProject()
    {}
    ~SpellCheckerCorePrivate() {}
};
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

using namespace SpellChecker;
using namespace SpellChecker::Internal;

static SpellCheckerCore *g_instance = 0;

SpellCheckerCore::SpellCheckerCore(QObject *parent) :
    QObject(parent),
    d(new Internal::SpellCheckerCorePrivate())
{
    Q_ASSERT(g_instance == NULL);
    g_instance = this;

    d->settings = new SpellCheckerCoreSettings();
    d->settings->loadFromSettings(Core::ICore::settings());
    d->spellingMistakesModel = new ProjectMistakesModel();

    d->mistakesModel = new SpellingMistakesModel(this);
    d->mistakesModel->setCurrentSpellingMistakes(WordList());
    connect(this, &SpellCheckerCore::activeProjectChanged, d->mistakesModel, &SpellingMistakesModel::setActiveProject);

    d->outputPane = new OutputPane(d->mistakesModel, this);
    connect(d->spellingMistakesModel, &ProjectMistakesModel::editorOpened, [=]() { d->outputPane->popup(Core::IOutputPane::NoModeSwitch); });

    d->optionsPage = new SpellCheckerCoreOptionsPage(d->settings);

    /* Connect to the editor changed signal for the core to act on */
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::currentEditorChanged, this, &SpellCheckerCore::mangerEditorChanged);
    connect(editorManager, &Core::EditorManager::editorOpened, this, &SpellCheckerCore::editorOpened);
    connect(editorManager, &Core::EditorManager::editorAboutToClose, this, &SpellCheckerCore::editorAboutToClose);

    connect(ProjectExplorer::SessionManager::instance(), &ProjectExplorer::SessionManager::startupProjectChanged, this, &SpellCheckerCore::startupProjectChanged);
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), &ProjectExplorer::ProjectExplorerPlugin::fileListChanged, this, &SpellCheckerCore::projectsFilesChanged);

    d->contextMenu = Core::ActionManager::createMenu(Constants::CONTEXT_MENU_ID);
    Q_ASSERT(d->contextMenu != NULL);
    connect(this, &SpellCheckerCore::wordUnderCursorMistake, d->contextMenu->menu(), &QMenu::setEnabled);
}
//--------------------------------------------------

SpellCheckerCore::~SpellCheckerCore()
{
    d->settings->saveToSettings(Core::ICore::settings());
    delete d->settings;

    g_instance = NULL;
    delete d;
}
//--------------------------------------------------

SpellCheckerCore *SpellCheckerCore::instance()
{
    return g_instance;
}
//--------------------------------------------------

bool SpellCheckerCore::addDocumentParser(IDocumentParser *parser)
{
    /* Check that the parser was not already added. If it was, do not
     * add it again and return false. */
    if(d->documentParsers.contains(parser) == false) {
        d->documentParsers << parser;
        /* Connect all signals and slots between the parser and the core. */
        connect(this, &SpellCheckerCore::currentEditorChanged, parser, &IDocumentParser::setCurrentEditor);
        connect(this, &SpellCheckerCore::activeProjectChanged, parser, &IDocumentParser::setActiveProject);
        connect(parser, &IDocumentParser::spellcheckWordsParsed, this, &SpellCheckerCore::spellcheckWordsFromParser, Qt::DirectConnection);
        return true;
    }
    return false;
}
//--------------------------------------------------

void SpellCheckerCore::removeDocumentParser(IDocumentParser *parser)
{
    if(parser == NULL) {
        return;
    }
    /* Disconnect all signals between the parser and the core. */
    disconnect(this, &SpellCheckerCore::currentEditorChanged, parser, &IDocumentParser::setCurrentEditor);
    disconnect(this, &SpellCheckerCore::activeProjectChanged, parser, &IDocumentParser::setActiveProject);
    disconnect(parser, &IDocumentParser::spellcheckWordsParsed, this, &SpellCheckerCore::spellcheckWordsFromParser);
    /* Remove the parser from the Core. The removeOne() function is used since
     * the check in the addDocumentParser() would prevent the list from having
     * more than one occurrence of the parser in the list of parsers */
    d->documentParsers.removeOne(parser);
}
//--------------------------------------------------

void SpellCheckerCore::addMisspelledWords(const QString &fileName, const WordList &words)
{
    d->spellingMistakesModel->insertSpellingMistakes(fileName, words, d->filesInStartupProject.contains(fileName));
    if(d->currentFilePath == fileName) {
        d->mistakesModel->setCurrentSpellingMistakes(words);
    }

    /* Only apply the underlines to the current file. This is done so that if the
     * whole project is scanned, it does not add selections to pages that might
     * potentially never be opened. This can especially be a problem in large
     * projects.
     */
    if(d->currentFilePath != fileName) {
        return;
    }
    TextEditor::BaseTextEditor* baseEditor = qobject_cast<TextEditor::BaseTextEditor*>(d->currentEditor);
    if(baseEditor == NULL) {
        return;
    }
    TextEditor::TextEditorWidget* editorWidget = baseEditor->editorWidget();
    if(editorWidget == NULL) {
        return;
    }
    QList<QTextEdit::ExtraSelection> selections;
    foreach (const Word& word, words.values()) {
        QTextCursor cursor(editorWidget->document());
        /* Walk to the correct position using the line and column number since the
         * absolute position is not available and I do not know of a way to get/
         * calculate the absolute position from that information.
         *
         * One would think that the position from the CppDocumentParser::tokenizeWords()
         * function can be used if stored in the Word, but it is not the correct position. */
        cursor.setPosition(0);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, word.lineNumber - 1);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, word.columnNumber - 1);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, word.length);

        /* Get the current format from the cursor, this is to make sure that the text font
         * and color stays the same, we just want to underline the mistake. */
        QTextCharFormat format = cursor.charFormat();
        format.setFontUnderline(true);
        format.setUnderlineColor(QColor(Qt::red));
        format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        format.setToolTip(word.suggestions.isEmpty() ? QLatin1String("Incorrect spelling")
                                                     : QString(QLatin1String("Incorrect spelling, did you mean '%1' ?")).arg(word.suggestions.first()));
        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        selection.format = format;
        selections.append(selection);
    }
    editorWidget->setExtraSelections(Core::Id(SpellChecker::Constants::SPELLCHECK_MISTAKE_ID), selections);
}
//--------------------------------------------------

OutputPane *SpellCheckerCore::outputPane() const
{
    return d->outputPane;
}
//--------------------------------------------------

ISpellChecker *SpellCheckerCore::spellChecker() const
{
    Q_ASSERT(d->spellChecker != NULL);
    return d->spellChecker;
}
//--------------------------------------------------

QMap<QString, ISpellChecker *> SpellCheckerCore::addedSpellCheckers() const
{
    return d->addedSpellCheckers;
}
//--------------------------------------------------

void SpellCheckerCore::addSpellChecker(ISpellChecker *spellChecker)
{
    if(spellChecker == NULL) {
        return;
    }

    if(d->addedSpellCheckers.contains(spellChecker->name()) == true) {
        return;
    }

    d->addedSpellCheckers.insert(spellChecker->name(), spellChecker);

    /* If none is set, set it to the one added */
    if(d->spellChecker == NULL) {
        setSpellChecker(spellChecker);
    }
}
//--------------------------------------------------

void SpellCheckerCore::setSpellChecker(ISpellChecker *spellChecker)
{
    if(d->spellChecker != NULL) {
        /* Disconnect all signals connected to the current set spellchecker */
        connect(this, &SpellCheckerCore::spellcheckWords, d->spellChecker, &ISpellChecker::spellcheckWords, Qt::DirectConnection);
        connect(d->spellChecker, &ISpellChecker::misspelledWordsForFile, this, &SpellCheckerCore::addMisspelledWords);
    }
    if(spellChecker == NULL) {
        return;
    }

    if(d->addedSpellCheckers.contains(spellChecker->name()) == false) {
        d->addedSpellCheckers.insert(spellChecker->name(), spellChecker);
    }

    d->spellChecker = spellChecker;
    /* Connect the signals between the core and the spellchecker. */
    connect(this, &SpellCheckerCore::spellcheckWords, d->spellChecker, &ISpellChecker::spellcheckWords, Qt::DirectConnection);
    connect(d->spellChecker, &ISpellChecker::misspelledWordsForFile, this, &SpellCheckerCore::addMisspelledWords);
}
//--------------------------------------------------

void SpellCheckerCore::spellcheckWordsFromParser(const QString& fileName, const WordList &words)
{
    emit spellcheckWords(fileName, words);
}
//--------------------------------------------------

Core::IOptionsPage *SpellCheckerCore::optionsPage()
{
    return d->optionsPage;
}
//--------------------------------------------------

SpellCheckerCoreSettings *SpellCheckerCore::settings() const
{
    return d->settings;
}
//--------------------------------------------------

ProjectMistakesModel *SpellCheckerCore::spellingMistakesModel() const
{
    return d->spellingMistakesModel;
}
//--------------------------------------------------

bool SpellCheckerCore::isWordUnderCursorMistake(Word& word)
{
    if(d->currentEditor.isNull() == true) {
        return false;
    }

    unsigned int column = d->currentEditor->currentColumn();
    unsigned int line = d->currentEditor->currentLine();
    QString currentFileName = d->currentEditor->document()->filePath().toString();
    WordList wl;
    wl = d->spellingMistakesModel->mistakesForFile(currentFileName);
    if(wl.isEmpty() == true) {
        return false;
    }
    WordList::ConstIterator iter = wl.constBegin();
    while(iter != wl.constEnd()) {
        const Word& currentWord = iter.value();
        if((currentWord.lineNumber == line)
                && ((currentWord.columnNumber <= column)
                    && (currentWord.columnNumber + currentWord.length) >= column)) {
            word = currentWord;
            return true;
        }
        ++iter;
    }
    return false;
}
//--------------------------------------------------

bool SpellCheckerCore::getAllOccurrencesOfWord(const Word &word, WordList& words)
{
    if(d->currentEditor.isNull() == true) {
        return false;
    }
    WordList wl;
    QString currentFileName = d->currentEditor->document()->filePath().toString();
    wl = d->spellingMistakesModel->mistakesForFile(currentFileName);
    if(wl.isEmpty() == true) {
        return false;
    }
    WordList::ConstIterator iter = wl.constBegin();
    while(iter != wl.constEnd()) {
        const Word& currentWord = iter.value();
        if(currentWord.text == word.text) {
            words.append(currentWord);
        }
        ++iter;
    }
    return (wl.count() > 0);

}
//--------------------------------------------------

void SpellCheckerCore::giveSuggestionsForWordUnderCursor()
{
    if(d->currentEditor.isNull() == true) {
        return;
    }
    Word word;
    WordList wordsToReplace;
    bool wordMistake = isWordUnderCursorMistake(word);
    if(wordMistake == false) {
        return;
    }

    getAllOccurrencesOfWord(word, wordsToReplace);

    SuggestionsDialog dialog(word.text, word.suggestions, wordsToReplace.count());
    SuggestionsDialog::ReturnCode code = static_cast<SuggestionsDialog::ReturnCode>(dialog.exec());
    switch(code) {
    case SuggestionsDialog::Rejected:
        /* Cancel and exit */
        return;
    case SuggestionsDialog::Accepted:
        /* Clear the list and only add the one to replace */
        wordsToReplace.clear();
        wordsToReplace.append(word);
        break;
    case SuggestionsDialog::AcceptAll:
        /* Do nothing since the list of words is already valid */
        break;
    default:
        Q_ASSERT(false);
        return;
    }

    QString replacement = dialog.replacementWord();
    replaceWordsInCurrentEditor(wordsToReplace, replacement);
}
//--------------------------------------------------

void SpellCheckerCore::ignoreWordUnderCursor()
{
    removeWordUnderCursor(Ignore);
}
//--------------------------------------------------

void SpellCheckerCore::addWordUnderCursor()
{
    removeWordUnderCursor(Add);
}
//--------------------------------------------------

void SpellCheckerCore::replaceWordUnderCursorFirstSuggestion() {
    Word word;
    /* Get the word under the cursor. */
    bool wordMistake = isWordUnderCursorMistake(word);
    if(wordMistake == false) {
        Q_ASSERT(wordMistake);
        return;
    }
    if(word.suggestions.isEmpty() == true) {
        /* Word does not have any suggestions */
        return;
    }
    WordList words;
    words.append(word);
    replaceWordsInCurrentEditor(words, word.suggestions.first());
}
//--------------------------------------------------

void SpellCheckerCore::cursorPositionChanged()
{
    /* Check if the cursor is over a spelling mistake */
    Word word;
    bool wordIsMisspelled = isWordUnderCursorMistake(word);
    emit wordUnderCursorMistake(wordIsMisspelled, word);
}
//--------------------------------------------------

void SpellCheckerCore::removeWordUnderCursor(RemoveAction action)
{
    if(d->currentEditor.isNull() == true) {
        return;
    }
    if(d->spellChecker == NULL) {
        return;
    }
    QString currentFileName = d->currentEditor->document()->filePath().toString();
    if(d->spellingMistakesModel->indexOfFile(currentFileName) == -1) {
        return;
    }
    Word word;
    bool wordMistake = isWordUnderCursorMistake(word);
    bool wordRemoved = false;
    if(wordMistake == true) {
        QString wordToRemove = word.text;
        switch(action) {
        case Ignore:
            wordRemoved = d->spellChecker->ignoreWord(wordToRemove);
            break;
        case Add:
            wordRemoved = d->spellChecker->addWord(wordToRemove);
            break;
        default:
            break;
        }
    }

    if(wordRemoved == true) {
        /* Remove all occurrences of the removed word. This removes the need to
         * reparse the whole project, it will be a lot faster doing this.  */
        d->spellingMistakesModel->removeAllOccurrences(word.text);
        /* Get the updated list associated with the file. */
        WordList newList = d->spellingMistakesModel->mistakesForFile(currentFileName);
        /* Re-add the mistakes for the file. This is at the moment a doing the same
         * thing twice, but until the 2 mistakes models are not combined this will be
         * needed for the mistakes in the  output pane to update. */
        addMisspelledWords(currentFileName, newList);
        /* Since the word is now removed from the list of spelling mistakes,
         * the word under the cursor is not a spelling mistake anymore. Notify
         * this. */
        emit wordUnderCursorMistake(false);
    }
    return;
}
//--------------------------------------------------

void SpellCheckerCore::replaceWordsInCurrentEditor(const WordList &wordsToReplace, const QString &replacementWord)
{
    if(wordsToReplace.count() == 0) {
        Q_ASSERT(wordsToReplace.count() != 0);
        return;
    }
    if(d->currentEditor == NULL) {
        Q_ASSERT(d->currentEditor != NULL);
        return;
    }
    TextEditor::TextEditorWidget* editorWidget = qobject_cast<TextEditor::TextEditorWidget*>(d->currentEditor->widget());
    if(editorWidget == NULL) {
        Q_ASSERT(editorWidget != NULL);
        return;
    }

    QTextCursor cursor = editorWidget->textCursor();
    /* Iterate the words and replace all one by one */
    foreach(const Word& wordToReplace, wordsToReplace) {
        editorWidget->gotoLine(wordToReplace.lineNumber, wordToReplace.columnNumber - 1);
        int wordStartPos = editorWidget->textCursor().position();
        editorWidget->gotoLine(wordToReplace.lineNumber, wordToReplace.columnNumber + wordToReplace.length - 1);
        int wordEndPos = editorWidget->textCursor().position();

        cursor.beginEditBlock();
        cursor.setPosition(wordStartPos);
        cursor.setPosition(wordEndPos, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(replacementWord);
        cursor.endEditBlock();
    }
}
//--------------------------------------------------

void SpellCheckerCore::startupProjectChanged(ProjectExplorer::Project *startupProject)
{
    d->spellingMistakesModel->clearAllSpellingMistakes();
    d->filesInStartupProject.clear();
    d->startupProject = startupProject;
    if(startupProject != NULL) {
        /* Check if the current project is not set to be ignored by the settings. */
        if(d->settings->projectsToIgnore.contains(startupProject->displayName()) == false) {
            d->filesInStartupProject = startupProject->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
        } else {
            /* The Project should be ignored and not be spell checked. */
            d->startupProject = NULL;
        }
    }
    emit activeProjectChanged(startupProject);
}
//--------------------------------------------------

void SpellCheckerCore::projectsFilesChanged()
{
    startupProjectChanged(d->startupProject);
}
//--------------------------------------------------

void SpellCheckerCore::mangerEditorChanged(Core::IEditor *editor)
{
    d->currentFilePath = QLatin1String("");
    d->currentEditor = editor;
    if(editor != NULL) {
        d->currentFilePath = editor->document()->filePath().toString();
    }

    emit currentEditorChanged(d->currentFilePath);

    WordList wl;
    if(d->currentFilePath.isEmpty() == false) {
        wl = d->spellingMistakesModel->mistakesForFile(d->currentFilePath);
    }
    d->mistakesModel->setCurrentSpellingMistakes(wl);
}
//--------------------------------------------------

void SpellCheckerCore::editorOpened(Core::IEditor *editor)
{
    if(editor == NULL) {
        return;
    }
    connect(qobject_cast<QPlainTextEdit*>(editor->widget()), &QPlainTextEdit::cursorPositionChanged, this, &SpellCheckerCore::cursorPositionChanged);
}
//--------------------------------------------------

void SpellCheckerCore::editorAboutToClose(Core::IEditor *editor)
{
    if(editor == NULL) {
        return;
    }
    disconnect(qobject_cast<QPlainTextEdit*>(editor->widget()), &QPlainTextEdit::cursorPositionChanged, this, &SpellCheckerCore::cursorPositionChanged);
}
//--------------------------------------------------
