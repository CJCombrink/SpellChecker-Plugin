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

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <texteditor/basetexteditor.h>

#include <QPointer>
#include <QMouseEvent>
#include <QTextCursor>


class SpellChecker::Internal::SpellCheckerCorePrivate {
public:

    QList<QPointer<SpellChecker::IDocumentParser> > documentParsers;
    QHash<QString /* fileName */, WordList> spellingMistakes;
    SpellChecker::Internal::SpellingMistakesModel* mistakesModel;
    SpellChecker::Internal::OutputPane* outputPane;
    SpellChecker::Internal::SpellCheckerCoreSettings* settings;
    SpellChecker::Internal::SpellCheckerCoreOptionsPage* optionsPage;
    QMap<QString, ISpellChecker*> addedSpellCheckers;
    SpellChecker::ISpellChecker* spellChecker;
    Core::IEditor *currentEditor;
    Core::ActionContainer *contextMenu;

    SpellCheckerCorePrivate() :
        spellChecker(NULL),
        currentEditor(NULL)
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

    d->mistakesModel = new SpellingMistakesModel(this);
    d->mistakesModel->setCurrentSpellingMistakes(WordList());

    d->outputPane = new OutputPane(d->mistakesModel, this);

    d->optionsPage = new SpellCheckerCoreOptionsPage(d->settings);

    /* Connect to the editor changed signal for the core to act on */
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)), this, SLOT(currentEditorChanged(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)), this, SLOT(editorOpened(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)), this, SLOT(editorAboutToClose(Core::IEditor*)));

    connect(ProjectExplorer::SessionManager::instance(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)), this, SLOT(startupProjectChanged(ProjectExplorer::Project*)));

    d->contextMenu = Core::ActionManager::createMenu(Constants::CONTEXT_MENU_ID);
    Q_ASSERT(d->contextMenu != NULL);
    connect(this, SIGNAL(wordUnderCursorMistake(bool)), d->contextMenu->menu(), SLOT(setEnabled(bool)));
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
    if(d->documentParsers.contains(parser) == false) {
        d->documentParsers << parser;
        return true;
    }
    return false;
}
//--------------------------------------------------

void SpellCheckerCore::removeDocumentParser(IDocumentParser *parser)
{
    /* Remove the parser from the Core. The removeOne() function is used since
     * the check in the addDocumentParser() would prevent the list from having
     * more than one occurrence of the parser in the list of parsers */
    d->documentParsers.removeOne(parser);
}
//--------------------------------------------------

void SpellCheckerCore::addWordsWithSpellingMistakes(const QString &fileName, const WordList &words)
{
    /* Remove the spelling mistakes for the given file name. This is done because
     * the new words should replace the old ones, and if there is no spelling mistakes
     * for the given file, the old ones get removed. */
    d->spellingMistakes.remove(fileName);
    if(words.count() != 0) {
        d->spellingMistakes.insert(fileName, words);
    }

    /* If the words belong to a file that is not the current open editor, do
     * nothing. Otherwise if it does belong to the current editor then update
     * the model to display the spelling mistakes on the open editor */
    if(d->currentEditor == NULL) {
        return;
    }
    if(d->currentEditor->document()->filePath() == fileName) {
        d->mistakesModel->setCurrentSpellingMistakes(words);
    }
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

void SpellCheckerCore::spellCheckWords(const QString& comment, WordList &words)
{
    if(d->spellChecker == NULL) {
        Q_ASSERT(d->spellChecker != NULL);
        return;
    }
    WordList::Iterator iter = words.begin();
    while(iter != words.end()) {
        bool spellingMistake = d->spellChecker->isSpellingMistake((*iter).text);
        /* Check to see if the char after the word is a period. If it is,
         * add the period to the word an see if it passes the checker. */
        if((spellingMistake == true)
                && ((*iter).end < comment.length())
                && (comment.at((*iter).end) == QLatin1Char('.'))) {
            /* Recheck the word with the period added */
            spellingMistake = d->spellChecker->isSpellingMistake((*iter).text + QLatin1Char('.'));
        }

        if(spellingMistake == true) {
            d->spellChecker->getSuggestionsForWord((*iter).text, (*iter).suggestions);
            ++iter;
        } else {
            /* Not a spelling mistake, remove it from the list of words */
            iter = words.erase(iter);
        }
    }
}
//--------------------------------------------------

Core::IOptionsPage *SpellCheckerCore::optionsPage()
{
    return d->optionsPage;
}
//--------------------------------------------------

bool SpellCheckerCore::isWordUnderCursorMistake(Word& word)
{
    if(d->currentEditor == NULL) {
        return false;
    }

    unsigned int column = d->currentEditor->currentColumn();
    unsigned int line = d->currentEditor->currentLine();
    QString currentFileName = d->currentEditor->document()->filePath();
    if(d->spellingMistakes.contains(currentFileName) == false) {
        return false;
    }
    WordList wl;
    wl = d->spellingMistakes.value(currentFileName);
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
    if(d->currentEditor == NULL) {
        return false;
    }
    WordList wl;
    QString currentFileName = d->currentEditor->document()->filePath();
    if(d->spellingMistakes.contains(currentFileName) == false) {
        return false;
    }
    wl = d->spellingMistakes.value(currentFileName);
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
    if(d->currentEditor == NULL) {
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

    d->currentEditor->document();
    d->currentEditor->widget();
    TextEditor::BaseTextEditorWidget* editorWidget = qobject_cast<TextEditor::BaseTextEditorWidget*>(d->currentEditor->widget());
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

        cursor.setPosition(wordStartPos);
        cursor.setPosition(wordEndPos, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(replacement);
    }
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
    if(d->currentEditor == NULL) {
        return;
    }
    if(d->spellChecker == NULL) {
        return;
    }
    QString currentFileName = d->currentEditor->document()->filePath();
    if(d->spellingMistakes.contains(currentFileName) == false) {
        return;
    }
    Word word;
    bool wordMistake = isWordUnderCursorMistake(word);
    if(wordMistake == true) {
        QString wordToRemove = word.text;
        /* TODO: Instead of removing just for this file, it might be
         * beneficial to remove this word from all files. When doing
         * this, attempt to do it in place... */
        WordList wl;
        wl = d->spellingMistakes.value(currentFileName);
        wl.remove(wordToRemove);
        addWordsWithSpellingMistakes(currentFileName, wl);
        switch(action) {
        case Ignore:
            d->spellChecker->ignoreWord(wordToRemove);
            break;
        case Add:
            d->spellChecker->addWord(wordToRemove);
            break;
        default:
            break;
        }
    }
    return;
}
//--------------------------------------------------

void SpellCheckerCore::startupProjectChanged(ProjectExplorer::Project *startupProject)
{
    QList<QPointer<SpellChecker::IDocumentParser> >::Iterator iter;
    for(iter = d->documentParsers.begin(); iter != d->documentParsers.end(); ++iter) {
        if((*iter).isNull() == true) {
            continue;
        }
        (*iter)->setStartupProject(startupProject);
    }
}
//--------------------------------------------------

void SpellCheckerCore::currentEditorChanged(Core::IEditor *editor)
{
    QList<QPointer<SpellChecker::IDocumentParser> >::Iterator iter;
    for(iter = d->documentParsers.begin(); iter != d->documentParsers.end(); ++iter) {
        if((*iter).isNull() == true) {
            continue;
        }
        (*iter)->setCurrentEditor(editor);
    }

    WordList wl;
    d->currentEditor = editor;
    if(editor != NULL) {
        QString currentFileName = editor->document()->filePath();
        if(d->spellingMistakes.contains(currentFileName) == true) {
            wl = d->spellingMistakes.value(currentFileName);
        }
    }
    d->mistakesModel->setCurrentSpellingMistakes(wl);
}
//--------------------------------------------------

void SpellCheckerCore::editorOpened(Core::IEditor *editor)
{
    if(editor == NULL) {
        return;
    }
    connect(editor->widget(), SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
}
//--------------------------------------------------

void SpellCheckerCore::editorAboutToClose(Core::IEditor *editor)
{
    if(editor == NULL) {
        return;
    }
    connect(editor->widget(), SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
}
//--------------------------------------------------
