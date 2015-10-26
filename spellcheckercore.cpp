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
#include <utils/QtConcurrentTools>
#include <utils/fadingindicator.h>

#include <QPointer>
#include <QMouseEvent>
#include <QTextCursor>
#include <QMenu>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>

typedef QMap<QFutureWatcher<SpellChecker::WordList>*, QString> FutureWatcherMap;
typedef FutureWatcherMap::Iterator FutureWatcherMapIter;

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
    QList<Core::Command*> contextMenuHolderCommands;
    QString currentFilePath;
    ProjectExplorer::Project* startupProject;
    QStringList filesInStartupProject;
    QMutex futureMutex;
    FutureWatcherMap futureWatchers;
    QStringList filesInProcess;
    QHash<QString, WordList> filesWaitingForProcess;

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
    connect(d->contextMenu->menu(), &QMenu::aboutToShow, this, &SpellCheckerCore::updateContextMenu);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &SpellCheckerCore::cancelFutures, Qt::DirectConnection);

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
        connect(parser, &IDocumentParser::spellcheckWordsParsed, this, &SpellCheckerCore::spellcheckWordsFromParser, Qt::QueuedConnection);
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
    if(spellChecker == NULL) {
        return;
    }

    if(d->addedSpellCheckers.contains(spellChecker->name()) == false) {
        d->addedSpellCheckers.insert(spellChecker->name(), spellChecker);
    }

    d->spellChecker = spellChecker;
}
//--------------------------------------------------

void SpellCheckerCore::spellcheckWordsFromParser(const QString& fileName, const WordList &words)
{
    /* Lock the mutex to prevent threading issues. This might not be needed since
     * queued connections are used and this function should always execute in the
     * main thread, but for now lets rather be safe. */
    QMutexLocker locker(&d->futureMutex);

    /* Check if this file is not already being processed by QtConcurrent in the
     * background. The current implementation will only use one QFuter per file
     * and if spell checking is requested for the same file if it is already being
     * spell checked, the file will get added to a list that will only be checked
     * if the current QFuture completes. This prevents possible redundant spell checking
     * but can result in a bit of a latency to update new words. It will however reduce
     * the amount of processing, especially if code is edited, and not comments and
     * literals. */
    if(d->filesInProcess.contains(fileName) == true) {
        /* There is already a QFuture out for the given file. Add it to the list of
         * of waiting files and replace the current set of words with the latest ones.
         * The assumption is that the last call to this function will always contain
         * the latest words that should be spell checked. */
        d->filesWaitingForProcess[fileName] = words;
    } else {
        /* Get the list of mistakes that were extracted on the file during the last
         * run of the processing. */
        WordList previousMistakes = d->spellingMistakesModel->mistakesForFile(fileName);
        /* There is no background process processing the words for the given file.
         * Create a processor and start processing the spelling mistakes in the
         * background using QtConcurrent and a QFuture. */
        SpellCheckProcessor *processor = new SpellCheckProcessor(d->spellChecker, fileName, words, previousMistakes);
        QFutureWatcher<WordList> *watcher = new QFutureWatcher<WordList>();
        connect(watcher, &QFutureWatcher<WordList>::finished, this, &SpellCheckerCore::futureFinished, Qt::QueuedConnection);
        /* Keep track of the watchers that are busy and the file that it is working on.
         * Since all QFuterWatchers are connected to the same slot, this map is used
         * to map the correct watcher to the correct file. */
        d->futureWatchers.insert(watcher, fileName);
        /* This is just a convenience list to speed up checking if a file is getting
         * processed already. An alternative would be to iterate through the above map
         * and check where the value matches the file. This can be slow especially if
         * there are multiple watchers running. The separate list can use indexing and
         * other search technicians compared to the mentioned iteration search. */
        d->filesInProcess.append(fileName);
        /* Make sure that the processor gets cleaned up after it has finished processing
         * the words. */
        connect(watcher, &QFutureWatcher<WordList>::finished, processor, &SpellCheckProcessor::deleteLater);
        /* Run the processor in the background and set a watcher to monitor the progress. */
        QFuture<WordList> future = QtConcurrent::run(&SpellCheckProcessor::process, processor);
        watcher->setFuture(future);
    }
}
//--------------------------------------------------

void SpellCheckerCore::futureFinished()
{
    /* Get the watcher from the sender() of the signal that invoked this slot.
     * reinterpret_cast is used since qobject_cast is not valid of template
     * classes since the template class does not have the Q_OBJECT macro. */
    QFutureWatcher<WordList> *watcher = reinterpret_cast<QFutureWatcher<WordList>*>(sender());
    if(watcher == nullptr) {
        return;
    }
    if(watcher->isCanceled() == true) {
        /* Application is shutting down */
        return;
    }
    /* Get the list of words with spelling mistakes from the future. */
    WordList checkedWords = watcher->result();
    QMutexLocker locker(&d->futureMutex);
    /* Get the file name associated with this future and the misspelled
     * words. */
    FutureWatcherMapIter iter = d->futureWatchers.find(watcher);
    if(iter == d->futureWatchers.end()) {
        return;
    }
    QString fileName = iter.value();
    /* Remove the watcher from the list of running watchers and the file that
     * kept track of the file getting spell checked. */
    d->futureWatchers.erase(iter);
    d->filesInProcess.removeAll(fileName);
    /* Check if the file was scheduled for a re-check. As discussed previously,
     * if a spell check was requested for a file that had a future already in
     * progress, it was scheduled for a re-check as soon as the in progress one
     * completes. If it was scheduled, restart it using the normal slot. */
    QHash<QString, WordList>::iterator waitingIter = d->filesWaitingForProcess.find(fileName);
    if(waitingIter != d->filesWaitingForProcess.end()) {
        WordList wordsToSpellCheck = waitingIter.value();
        /* remove the file and words from the scheduled list. */
        d->filesWaitingForProcess.erase(waitingIter);
        locker.unlock();
        /* Invoke the method to make sure that it gets called from the main thread.
         * This will most probably be already in the main thread, but to make sure
         * it is done like this. */
        this->metaObject()->invokeMethod(this
                                         , "spellcheckWordsFromParser"
                                         , Qt::QueuedConnection
                                         , Q_ARG(QString, fileName)
                                         , Q_ARG(SpellChecker::WordList, wordsToSpellCheck));
    }
    locker.unlock();
    watcher->deleteLater();
    /* Add the list of misspelled words to the mistakes model */
    addMisspelledWords(fileName, checkedWords);
}
//--------------------------------------------------

void SpellCheckerCore::cancelFutures()
{
    QMutexLocker lock(&d->futureMutex);
    /* Iterate the futures and cancel them. */
    FutureWatcherMapIter iter = d->futureWatchers.begin();
    for(iter = d->futureWatchers.begin(); iter != d->futureWatchers.end(); ++iter) {
        iter.key()->future().cancel();
    }
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
    /* If more than one suggestion was replaced, show a notification */
    if(wordsToReplace.count() > 1) {
        Utils::FadingIndicator::showText(editorWidget,
                                         tr("%1 occurrences replaced.").arg(wordsToReplace.count()),
                                         Utils::FadingIndicator::SmallText);
    }
}
//--------------------------------------------------

void SpellCheckerCore::startupProjectChanged(ProjectExplorer::Project *startupProject)
{
    /* Cancel all outstanding futures */
    cancelFutures();
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

void SpellCheckerCore::updateContextMenu()
{
    if(d->contextMenuHolderCommands.isEmpty() == true) {
        /* Populate the internal vector with the holder actions to speed up the process
         * of updating the context menu when requested again. */
        QVector<const char *> holderActionIds {Constants::ACTION_HOLDER1_ID, Constants::ACTION_HOLDER2_ID, Constants::ACTION_HOLDER3_ID, Constants::ACTION_HOLDER4_ID, Constants::ACTION_HOLDER5_ID};
        /* Iterate the commands and */
        for(int count = 0 ; count < holderActionIds.size(); ++count) {;
            Core::Command* cmd = Core::ActionManager::command(holderActionIds[count]);
            d->contextMenuHolderCommands.push_back(cmd);
        }
    }

    Word word;
    bool isMistake = isWordUnderCursorMistake(word);
    /* Do nothing if the context menu is not a mistake.
     * The context menu will in this case already be disabled so there
     * is no need to update it. */
    if(isMistake == false) {
        return;
    }
    QStringList list = word.suggestions;
    /* Iterate the commands and */
    for(Core::Command* cmd: d->contextMenuHolderCommands) {
        Q_ASSERT(cmd != nullptr);
        if(list.size() > 0) {
            /* Disconnect the previous connection made, otherwise it will also trigger */
            cmd->action()->disconnect();
            /* Set the text on the action for the word to use*/
            cmd->action()->setText(list.takeFirst());
            /* Show the action */
            cmd->action()->setVisible(true);
            /* Connect to lambda function to call to replace the words if the
             * action is triggered. */
            connect(cmd->action(), &QAction::triggered, [this, word, cmd]() {
                WordList wordsToReplace;
                if(d->settings->replaceAllFromRightClick == true) {
                    this->getAllOccurrencesOfWord(word, wordsToReplace);
                } else {
                    wordsToReplace.append(word);
                }
                this->replaceWordsInCurrentEditor(wordsToReplace, cmd->action()->text());
            });
        } else {
            /* Hide the action since there are less than 5 suggestions for the word. */
            cmd->action()->setVisible(false);
        }
    }
}
//--------------------------------------------------
