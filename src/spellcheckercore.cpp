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
#include "ISpellChecker.h"
#include "NavigationWidget.h"
#include "outputpane.h"
#include "spellcheckerconstants.h"
#include "spellcheckercore.h"
#include "spellcheckercoreoptionswidget.h"
#include "spellcheckercoresettings.h"
#include "spellingmistakesmodel.h"
#include "suggestionsdialog.h"

#include <coreplugin/session.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <cppeditor/cppmodelmanager.h>
#include <texteditor/texteditor.h>
#include <utils/algorithm.h>
#include <utils/fadingindicator.h>
#include <utils/async.h>
#include <utils/fileutils.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QMenu>
#include <QMouseEvent>
#include <QMutex>
#include <QPointer>
#include <QtConcurrent>
#include <QTextBlock>
#include <QTextCursor>

using FutureWatcherMap     = QMap<QFutureWatcher<SpellChecker::WordList>*, QString>;
using FutureWatcherMapIter = FutureWatcherMap::Iterator;

class SpellChecker::Internal::SpellCheckerCorePrivate
{
public:

  QList<QPointer<SpellChecker::IDocumentParser>> documentParsers;
  ProjectMistakesModel* spellingMistakesModel;
  SpellChecker::Internal::SpellingMistakesModel* mistakesModel;
  SpellChecker::Internal::OutputPane* outputPane;
  SpellChecker::Internal::SpellCheckerCoreSettings settings;
  SpellChecker::Internal::SpellCheckerCoreOptionsPage optionsPage{&settings, [this] { /*settingsChanged(settings);*/ }};
  QMap<QString, ISpellChecker*> addedSpellCheckers;
  SpellChecker::ISpellChecker*  spellChecker;
  QPointer<Core::IEditor> currentEditor;
  Core::ActionContainer*  contextMenu;
  QList<Core::Command*> contextMenuHolderCommands;
  QString currentFilePath;
  ProjectExplorer::Project* startupProject;
  QStringSet filesInStartupProject;
  QMutex futureMutex;
  FutureWatcherMap futureWatchers;
  QStringList filesInProcess;
  QHash<QString, WordList> filesWaitingForProcess;
  bool shuttingDown = false;

  SpellCheckerCorePrivate()
    : spellChecker( nullptr )
    , currentEditor( nullptr )
    , currentFilePath()
    , startupProject( nullptr )
    , filesInStartupProject()
  {}
  ~SpellCheckerCorePrivate() {}
};
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

using namespace SpellChecker;
using namespace SpellChecker::Internal;

static SpellCheckerCore* g_instance = nullptr;

SpellCheckerCore::SpellCheckerCore( QObject* parent )
  : QObject( parent )
  , d( new Internal::SpellCheckerCorePrivate() )
{
  Q_ASSERT( g_instance == nullptr );
  g_instance = this;

  d->settings.loadFromSettings( Core::ICore::settings() );
  d->spellingMistakesModel = new ProjectMistakesModel();

  d->mistakesModel = new SpellingMistakesModel( this );
  d->mistakesModel->setCurrentSpellingMistakes( WordList() );
  connect( this, &SpellCheckerCore::activeProjectChanged, d->mistakesModel, &SpellingMistakesModel::setActiveProject );

  d->outputPane = new OutputPane( d->mistakesModel, this );
  connect( d->spellingMistakesModel, &ProjectMistakesModel::editorOpened, d->outputPane, [=]() { d->outputPane->popup( Core::IOutputPane::NoModeSwitch ); } );

  /* Connect to the editor changed signal for the core to act on */
  Core::EditorManager* editorManager = Core::EditorManager::instance();
  connect( editorManager, &Core::EditorManager::currentEditorChanged, this, &SpellCheckerCore::mangerEditorChanged );
  connect( editorManager, &Core::EditorManager::editorOpened,         this, &SpellCheckerCore::editorOpened );
  connect( editorManager, &Core::EditorManager::editorAboutToClose,   this, &SpellCheckerCore::editorAboutToClose );

  connect( ProjectExplorer::ProjectManager::instance(),        &ProjectExplorer::ProjectManager::startupProjectChanged,  this, &SpellCheckerCore::startupProjectChanged );
  connect( ProjectExplorer::ProjectExplorerPlugin::instance(), &ProjectExplorer::ProjectExplorerPlugin::fileListChanged, this, &SpellCheckerCore::fileListChanged );

  d->contextMenu = Core::ActionManager::createMenu( Constants::CONTEXT_MENU_ID );
  Q_ASSERT( d->contextMenu != nullptr );
  connect( d->contextMenu->menu(), &QMenu::aboutToShow,            this, &SpellCheckerCore::updateContextMenu );
  connect( qApp,                   &QCoreApplication::aboutToQuit, this, &SpellCheckerCore::aboutToQuit, Qt::DirectConnection );

  connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
          this, [this] { d->settings.saveToSettings(Core::ICore::settings()); });
}
// --------------------------------------------------

SpellCheckerCore::~SpellCheckerCore()
{
  delete d->outputPane;

  g_instance = nullptr;
  delete d;
}
// --------------------------------------------------

SpellCheckerCore* SpellCheckerCore::instance()
{
  return g_instance;
}
// --------------------------------------------------

bool SpellCheckerCore::addDocumentParser( IDocumentParser* parser )
{
  /* Check that the parser was not already added. If it was, do not
   * add it again and return false. */
  if( d->documentParsers.contains( parser ) == false ) {
    d->documentParsers << parser;
    /* Connect all signals and slots between the parser and the core. */
    connect( this,   &SpellCheckerCore::currentEditorChanged, parser, &IDocumentParser::setCurrentEditor );
    connect( this,   &SpellCheckerCore::activeProjectChanged, parser, &IDocumentParser::setActiveProject );
    connect( this,   &SpellCheckerCore::projectFilesChanged,  parser, &IDocumentParser::updateProjectFiles );
    connect( parser, &IDocumentParser::spellcheckWordsParsed, this,   &SpellCheckerCore::spellcheckWordsFromParser, Qt::QueuedConnection );
    return true;
  }
  return false;
}
// --------------------------------------------------

void SpellCheckerCore::removeDocumentParser( IDocumentParser* parser )
{
  if( parser == nullptr ) {
    return;
  }
  /* Disconnect all signals between the parser and the core. */
  disconnect( this,   &SpellCheckerCore::currentEditorChanged, parser, &IDocumentParser::setCurrentEditor );
  disconnect( this,   &SpellCheckerCore::activeProjectChanged, parser, &IDocumentParser::setActiveProject );
  disconnect( this,   &SpellCheckerCore::projectFilesChanged,  parser, &IDocumentParser::updateProjectFiles );
  disconnect( parser, &IDocumentParser::spellcheckWordsParsed, this,   &SpellCheckerCore::spellcheckWordsFromParser );
  /* Remove the parser from the Core. The removeOne() function is used since
   * the check in the addDocumentParser() would prevent the list from having
   * more than one occurrence of the parser in the list of parsers */
  d->documentParsers.removeOne( parser );
}
// --------------------------------------------------

void SpellCheckerCore::addMisspelledWords( const QString& fileName, const WordList& words )
{
  d->spellingMistakesModel->insertSpellingMistakes( fileName, words, d->filesInStartupProject.contains( fileName ) );
  if( d->currentFilePath == fileName ) {
    d->mistakesModel->setCurrentSpellingMistakes( words );
  }

  /* Only apply the underlines to the current file. This is done so that if the
   * whole project is scanned, it does not add selections to pages that might
   * potentially never be opened. This can especially be a problem in large
   * projects.
   */
  if( d->currentFilePath != fileName ) {
    return;
  }
  TextEditor::BaseTextEditor* baseEditor = qobject_cast<TextEditor::BaseTextEditor*>( d->currentEditor );
  if( baseEditor == nullptr ) {
    return;
  }
  TextEditor::TextEditorWidget* editorWidget = baseEditor->editorWidget();
  if( editorWidget == nullptr ) {
    return;
  }
  QTextDocument* document = editorWidget->document();
  if( document == nullptr ) {
    return;
  }
  QList<QTextEdit::ExtraSelection> selections;
  selections.reserve( words.size() );
  const WordList::ConstIterator wordsEnd = words.constEnd();
  for( WordList::ConstIterator wordIter = words.constBegin(); wordIter != wordsEnd; ++wordIter ) {
    const Word& word = wordIter.value();
    /* Get the QTextBlock for the line that the misspelled word is on.
     * The QTextDocument manages lines as blocks (in most cases).
     * The lineNumber of the misspelled word is 1 based (seen in the editor)
     * but the blocks on the QTextDocument are 0 based, thus minus one
     * from the line number to get the correct line.
     * If the block is valid, and the word is not longer than the number of
     * characters in the block (which should normally not be the case)
     * then the cursor is moved to the correct column, and the word is
     * underlined.
     * Again the Columns on the misspelled word is 1 based but
     * the blocks and cursor are 0 based. */
    const QTextBlock& block = document->findBlockByNumber( int32_t( word.lineNumber ) - 1 );
    if( ( block.isValid() == false )
        || ( uint32_t( block.length() ) < ( word.columnNumber - 1 + uint32_t( word.length ) ) ) ) {
      continue;
    }

    QTextCursor cursor( block );
    cursor.setPosition( cursor.position() + int32_t( word.columnNumber ) - 1 );
    cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor, word.length );
    /* Get the current format from the cursor, this is to make sure that the text font
     * and color stays the same, we just want to underline the mistake. */
    QTextCharFormat format = cursor.charFormat();
    format.setFontUnderline( true );
    format.setUnderlineColor( d->settings.errorsColor );
    format.setUnderlineStyle( QTextCharFormat::WaveUnderline );
    format.setToolTip( word.suggestions.isEmpty()
                       ? QStringLiteral( "Incorrect spelling" )
                       : QStringLiteral( "Incorrect spelling, did you mean '%1' ?" ).arg( word.suggestions.first() ) );
    QTextEdit::ExtraSelection selection;
    selection.cursor = cursor;
    selection.format = format;
    selections.append( selection );
  }
  editorWidget->setExtraSelections( Utils::Id( SpellChecker::Constants::SPELLCHECK_MISTAKE_ID ), selections );

  /* The model updated, check if the word under the cursor is now a mistake
   * and notify the rest of the checker with this information. */
  Word word;
  bool wordIsMisspelled = isWordUnderCursorMistake( word );
  emit wordUnderCursorMistake( wordIsMisspelled, word );
}
// --------------------------------------------------

OutputPane* SpellCheckerCore::outputPane() const
{
  return d->outputPane;
}
// --------------------------------------------------

ISpellChecker* SpellCheckerCore::spellChecker() const
{
  Q_ASSERT( d->spellChecker != nullptr );
  return d->spellChecker;
}
// --------------------------------------------------

QMap<QString, ISpellChecker*> SpellCheckerCore::addedSpellCheckers() const
{
  return d->addedSpellCheckers;
}
// --------------------------------------------------

void SpellCheckerCore::addSpellChecker( ISpellChecker* spellChecker )
{
  if( spellChecker == nullptr ) {
    return;
  }

  if( d->addedSpellCheckers.contains( spellChecker->name() ) == true ) {
    return;
  }

  d->addedSpellCheckers.insert( spellChecker->name(), spellChecker );

  /* If none is set, set it to the one added */
  if( d->spellChecker == nullptr ) {
    setSpellChecker( spellChecker );
  }
}
// --------------------------------------------------

void SpellCheckerCore::setSpellChecker( ISpellChecker* spellChecker )
{
  if( spellChecker == nullptr ) {
    return;
  }

  if( d->addedSpellCheckers.contains( spellChecker->name() ) == false ) {
    d->addedSpellCheckers.insert( spellChecker->name(), spellChecker );
  }

  d->spellChecker = spellChecker;
}
// --------------------------------------------------

void SpellCheckerCore::spellcheckWordsFromParser( const QString& fileName, const WordList& words )
{
  /* Lock the mutex to prevent threading issues. This might not be needed since
   * queued connections are used and this function should always execute in the
   * main thread, but for now lets rather be safe. */
  QMutexLocker locker( &d->futureMutex );
  if( d->shuttingDown == true ) {
    /* Shutting down, no need to do anything further. */
    return;
  }

  /* Check if this file is not already being processed by QtConcurrent in the
   * background. The current implementation will only use one QFuter per file
   * and if spell checking is requested for the same file if it is already being
   * spell checked, the file will get added to a list that will only be checked
   * if the current QFuture completes. This prevents possible redundant spell checking
   * but can result in a bit of a latency to update new words. It will however reduce
   * the amount of processing, especially if code is edited, and not comments and
   * literals. */
  if( d->filesInProcess.contains( fileName ) == true ) {
    /* There is already a QFuture out for the given file. Add it to the list of
     * of waiting files and replace the current set of words with the latest ones.
     * The assumption is that the last call to this function will always contain
     * the latest words that should be spell checked. */
    d->filesWaitingForProcess[fileName] = words;
  } else {
    /* Get the list of mistakes that were extracted on the file during the last
     * run of the processing. */
    WordList previousMistakes = d->spellingMistakesModel->mistakesForFile( fileName );
    /* There is no background process processing the words for the given file.
     * Create a processor and start processing the spelling mistakes in the
     * background using QtConcurrent and a QFuture. */
    SpellCheckProcessor* processor    = new SpellCheckProcessor( d->spellChecker, fileName, words, previousMistakes );
    QFutureWatcher<WordList>* watcher = new QFutureWatcher<WordList>();
    connect( watcher, &QFutureWatcher<WordList>::finished, this, &SpellCheckerCore::futureFinished, Qt::QueuedConnection );
    /* Keep track of the watchers that are busy and the file that it is working on.
     * Since all QFuterWatchers are connected to the same slot, this map is used
     * to map the correct watcher to the correct file. */
    d->futureWatchers.insert( watcher, fileName );
    /* This is just a convenience list to speed up checking if a file is getting
     * processed already. An alternative would be to iterate through the above map
     * and check where the value matches the file. This can be slow especially if
     * there are multiple watchers running. The separate list can use indexing and
     * other search technicians compared to the mentioned iteration search. */
    d->filesInProcess.append( fileName );
    /* Make sure that the processor gets cleaned up after it has finished processing
     * the words. */
    connect( watcher, &QFutureWatcher<WordList>::finished, processor, &SpellCheckProcessor::deleteLater );

    /* Create a future to process the file.
     * If the file to process is the current open editor, it is processed in a new
     * thread with high priority.
     * If it is not the current file, it is added to the global thread pool
     * since it can get processed in its own time.
     * The current one gets a new thread so that it can get processed as
     * soon as possible and it does not need to get queued along with all other
     * futures added to the global thread pool. */
    if( fileName == d->currentFilePath ) {
      QFuture<WordList> future = Utils::asyncRun( QThread::HighPriority, &SpellCheckProcessor::process, processor );
      watcher->setFuture( future );
    } else {
      QFuture<WordList> future = Utils::asyncRun( QThreadPool::globalInstance(), QThread::LowPriority, &SpellCheckProcessor::process, processor );
      watcher->setFuture( future );
    }
  }
}
// --------------------------------------------------

void SpellCheckerCore::futureFinished()
{
  /* Get the watcher from the sender() of the signal that invoked this slot.
   * reinterpret_cast is used since qobject_cast is not valid of template
   * classes since the template class does not have the Q_OBJECT macro. */
  QFutureWatcher<WordList>* watcher = reinterpret_cast<QFutureWatcher<WordList>*>( sender() );
  if( watcher == nullptr ) {
    return;
  }

  if( d->shuttingDown == true ) {
    /* Application shutting down, should not try something */
    return;
  }
  if( watcher->isCanceled() == true ) {
    /* Application is shutting down */
    return;
  }
  /* Get the list of words with spelling mistakes from the future. */
  WordList checkedWords = watcher->result();
  QMutexLocker locker( &d->futureMutex );
  /* Recheck again after getting the lock. */
  if( d->shuttingDown == true ) {
    return;
  }
  /* Get the file name associated with this future and the misspelled
   * words. */
  FutureWatcherMapIter iter = d->futureWatchers.find( watcher );
  if( iter == d->futureWatchers.end() ) {
    return;
  }
  QString fileName = iter.value();
  /* Remove the watcher from the list of running watchers and the file that
   * kept track of the file getting spell checked. */
  d->futureWatchers.erase( iter );
  d->filesInProcess.removeAll( fileName );
  /* Check if the file was scheduled for a re-check. As discussed previously,
   * if a spell check was requested for a file that had a future already in
   * progress, it was scheduled for a re-check as soon as the in progress one
   * completes. If it was scheduled, restart it using the normal slot. */
  QHash<QString, WordList>::iterator waitingIter = d->filesWaitingForProcess.find( fileName );
  if( waitingIter != d->filesWaitingForProcess.end() ) {
    WordList wordsToSpellCheck = waitingIter.value();
    /* remove the file and words from the scheduled list. */
    d->filesWaitingForProcess.erase( waitingIter );
    locker.unlock();
    /* Invoke the method to make sure that it gets called from the main thread.
     * This will most probably be already in the main thread, but to make sure
     * it is done like this. */
    this->metaObject()->invokeMethod( this
                                      , "spellcheckWordsFromParser"
                                      , Qt::QueuedConnection
                                      , Q_ARG( QString, fileName )
                                      , Q_ARG( SpellChecker::WordList, wordsToSpellCheck ) );
  } else {
    locker.unlock();
  }
  watcher->deleteLater();
  /* Add the list of misspelled words to the mistakes model */
  addMisspelledWords( fileName, checkedWords );
}
// --------------------------------------------------

void SpellCheckerCore::cancelFutures()
{
  QMutexLocker lock( &d->futureMutex );
  /* Iterate the futures and cancel them. */
  FutureWatcherMapIter iter = d->futureWatchers.begin();
  for( iter = d->futureWatchers.begin(); iter != d->futureWatchers.end(); ++iter ) {
    iter.key()->future().cancel();
  }

  /* Wait on the futures and delete the futures */
  for( iter = d->futureWatchers.begin(); iter != d->futureWatchers.end(); ++iter ) {
    iter.key()->future().waitForFinished();
    delete iter.key();
  }
  d->futureWatchers.clear();
}

// --------------------------------------------------
void SpellCheckerCore::aboutToQuit()
{
  /* Disconnect from everything that can send signals to this object */
  Core::EditorManager::instance()->disconnect( this );
  Core::SessionManager::instance()->disconnect( this );
  ProjectExplorer::ProjectExplorerPlugin::instance()->disconnect( this );
  d->shuttingDown   = true;
  d->startupProject = nullptr;
  disconnect( this );
  cancelFutures();
}
// --------------------------------------------------

Core::IOptionsPage* SpellCheckerCore::optionsPage()
{
  return &d->optionsPage;
}
// --------------------------------------------------

SpellCheckerCoreSettings* SpellCheckerCore::settings() const
{
  return &d->settings;
}
// --------------------------------------------------

ProjectMistakesModel* SpellCheckerCore::spellingMistakesModel() const
{
  return d->spellingMistakesModel;
}
// --------------------------------------------------

bool SpellCheckerCore::isWordUnderCursorMistake( Word& word ) const
{
  if( d->currentEditor.isNull() == true ) {
    return false;
  }

  int32_t column           = d->currentEditor->currentColumn();
  int32_t line             = d->currentEditor->currentLine();
  QString  currentFileName = d->currentEditor->document()->filePath().toString();
  WordList wl;
  wl = d->spellingMistakesModel->mistakesForFile( currentFileName );
  if( wl.isEmpty() == true ) {
    return false;
  }
  WordList::ConstIterator iter          = wl.constBegin();
  const WordList::ConstIterator iterEnd = wl.constEnd();
  while( iter != iterEnd ) {
    const Word& currentWord = iter.value();
    if( ( currentWord.lineNumber == line )
        && ( ( currentWord.columnNumber <= column )
             && ( ( currentWord.columnNumber + currentWord.length ) >= column ) ) ) {
      word = currentWord;
      return true;
    }
    ++iter;
  }
  return false;
}
// --------------------------------------------------

bool SpellCheckerCore::getAllOccurrencesOfWord( const Word& word, WordList& words )
{
  if( d->currentEditor.isNull() == true ) {
    return false;
  }
  WordList wl;
  QString  currentFileName = d->currentEditor->document()->filePath().toString();
  wl = d->spellingMistakesModel->mistakesForFile( currentFileName );
  if( wl.isEmpty() == true ) {
    return false;
  }
  WordList::ConstIterator iter = wl.constBegin();
  while( iter != wl.constEnd() ) {
    const Word& currentWord = iter.value();
    if( currentWord.text == word.text ) {
      words.append( currentWord );
    }
    ++iter;
  }
  return ( wl.count() > 0 );

}
// --------------------------------------------------

void SpellCheckerCore::giveSuggestionsForWordUnderCursor()
{
  if( d->currentEditor.isNull() == true ) {
    return;
  }
  Word word;
  WordList wordsToReplace;
  bool wordMistake = isWordUnderCursorMistake( word );
  if( wordMistake == false ) {
    return;
  }

  getAllOccurrencesOfWord( word, wordsToReplace );

  SuggestionsDialog dialog( word.text, word.suggestions, wordsToReplace.count() );
  SuggestionsDialog::ReturnCode code = static_cast<SuggestionsDialog::ReturnCode>( dialog.exec() );
  switch( code ) {
    case SuggestionsDialog::Rejected:
      /* Cancel and exit */
      return;
    case SuggestionsDialog::Accepted:
      /* Clear the list and only add the one to replace */
      wordsToReplace.clear();
      wordsToReplace.append( word );
      break;
    case SuggestionsDialog::AcceptAll:
      /* Do nothing since the list of words is already valid */
      break;
  }

  QString replacement = dialog.replacementWord();
  replaceWordsInCurrentEditor( wordsToReplace, replacement );
}
// --------------------------------------------------

void SpellCheckerCore::ignoreWordUnderCursor()
{
  removeWordUnderCursor( Ignore );
}
// --------------------------------------------------

void SpellCheckerCore::addWordUnderCursor()
{
  removeWordUnderCursor( Add );
}
// --------------------------------------------------

void SpellCheckerCore::replaceWordUnderCursorFirstSuggestion()
{
  Word word;
  /* Get the word under the cursor. */
  bool wordMistake = isWordUnderCursorMistake( word );
  if( wordMistake == false ) {
    Q_ASSERT( wordMistake );
    return;
  }
  if( word.suggestions.isEmpty() == true ) {
    /* Word does not have any suggestions */
    return;
  }
  WordList words;
  words.append( word );
  replaceWordsInCurrentEditor( words, word.suggestions.first() );
}
// --------------------------------------------------

void SpellCheckerCore::cursorPositionChanged()
{
  /* Check if the cursor is over a spelling mistake */
  Word word;
  bool wordIsMisspelled = isWordUnderCursorMistake( word );
  emit wordUnderCursorMistake( wordIsMisspelled, word );
}
// --------------------------------------------------

void SpellCheckerCore::removeWordUnderCursor( RemoveAction action )
{
  if( d->currentEditor.isNull() == true ) {
    return;
  }
  if( d->spellChecker == nullptr ) {
    return;
  }
  QString currentFileName = d->currentEditor->document()->filePath().toString();
  if( d->spellingMistakesModel->indexOfFile( currentFileName ) == -1 ) {
    return;
  }
  Word word;
  bool wordMistake = isWordUnderCursorMistake( word );
  bool wordRemoved = false;
  if( wordMistake == true ) {
    QString wordToRemove = word.text;
    switch( action ) {
      case Ignore:
        wordRemoved = d->spellChecker->ignoreWord( wordToRemove );
        break;
      case Add:
        wordRemoved = d->spellChecker->addWord( wordToRemove );
        break;
      default:
        break;
    }
  }

  if( wordRemoved == true ) {
    /* Remove all occurrences of the removed word. This removes the need to
     * re-parse the whole project, it will be a lot faster doing this.  */
    d->spellingMistakesModel->removeAllOccurrences( word.text );
    /* Get the updated list associated with the file. */
    WordList newList = d->spellingMistakesModel->mistakesForFile( currentFileName );
    /* Re-add the mistakes for the file. This is at the moment a doing the same
     * thing twice, but until the 2 mistakes models are not combined this will be
     * needed for the mistakes in the  output pane to update. */
    addMisspelledWords( currentFileName, newList );
    /* Since the word is now removed from the list of spelling mistakes,
     * the word under the cursor is not a spelling mistake anymore. Notify
     * this. */
    emit wordUnderCursorMistake( false );
  }
  return;
}
// --------------------------------------------------

void SpellCheckerCore::replaceWordsInCurrentEditor( const WordList& wordsToReplace, const QString& replacementWord )
{
  if( wordsToReplace.count() == 0 ) {
    Q_ASSERT( wordsToReplace.count() != 0 );
    return;
  }
  if( d->currentEditor == nullptr ) {
    Q_ASSERT( d->currentEditor != nullptr );
    return;
  }
  TextEditor::TextEditorWidget* editorWidget = qobject_cast<TextEditor::TextEditorWidget*>( d->currentEditor->widget() );
  if( editorWidget == nullptr ) {
    Q_ASSERT( editorWidget != nullptr );
    return;
  }

  QTextCursor cursor = editorWidget->textCursor();
  /* Iterate the words and replace all one by one */
  for( const Word& wordToReplace: wordsToReplace ) {
    editorWidget->gotoLine( int32_t( wordToReplace.lineNumber ), int32_t( wordToReplace.columnNumber ) - 1 );
    int wordStartPos = editorWidget->textCursor().position();
    editorWidget->gotoLine( int32_t( wordToReplace.lineNumber ), int32_t( wordToReplace.columnNumber ) + wordToReplace.length - 1 );
    int wordEndPos = editorWidget->textCursor().position();

    cursor.beginEditBlock();
    cursor.setPosition( wordStartPos );
    cursor.setPosition( wordEndPos, QTextCursor::KeepAnchor );
    cursor.removeSelectedText();
    cursor.insertText( replacementWord );
    cursor.endEditBlock();
  }
  /* If more than one suggestion was replaced, show a notification */
  if( wordsToReplace.count() > 1 ) {
    Utils::FadingIndicator::showText( editorWidget,
                                      tr( "%1 occurrences replaced." ).arg( wordsToReplace.count() ),
                                      Utils::FadingIndicator::SmallText );
  }
}
// --------------------------------------------------

void SpellCheckerCore::startupProjectChanged( ProjectExplorer::Project* startupProject )
{
  /* Cancel all outstanding futures */
  cancelFutures();
  d->spellingMistakesModel->clearAllSpellingMistakes();
  d->filesInStartupProject.clear();
  d->startupProject = startupProject;
  if( startupProject != nullptr ) {
    /* Check if the current project is not set to be ignored by the settings. */
    if( d->settings.projectsToIgnore.contains( startupProject->displayName() ) == false ) {
      const auto fileList = Utils::transform<QSet<QString>>( startupProject->files( ProjectExplorer::Project::SourceFiles ), &Utils::FilePath::toString );
      d->filesInStartupProject = fileList;
    } else {
      /* The Project should be ignored and not be spell checked. */
      d->startupProject = nullptr;
    }
  }
  emit activeProjectChanged( startupProject );
}
// --------------------------------------------------

void SpellCheckerCore::fileListChanged()
{
  if( d->startupProject == nullptr ) {
    return;
  }


  if( d->settings.projectsToIgnore.contains( d->startupProject->displayName() ) == true ) {
    /* We should ignore this project, return without doing anything. */
    return;
  }

  const QStringSet oldFiles = d->filesInStartupProject;
  const auto newFiles = Utils::transform<QStringSet>( d->startupProject->files( ProjectExplorer::Project::SourceFiles ), &Utils::FilePath::toString );

  /* Compare the two sets with each other to get the lists of files
   * added and removed.
   * An implementation using std::set_difference was initially implemented
   * but that needed the set to be converted to a vector so that it can be
   * sorted, then after std::set_difference the vector was converted back
   * to a set. This approach was in almost all tests cases slower than the
   * current implementation.
   *
   * The current implementation relies on the fact that searching in a set
   * is generally fast. */
  const QStringSet added = Utils::filtered( newFiles, [&]( const QString& file ) {
    return !oldFiles.contains( file );
  } );
  const QStringSet removed = Utils::filtered( oldFiles, [&]( const QString& file ) {
    return !newFiles.contains( file );
  } );

  d->filesInStartupProject = newFiles;
  /* Must let the model know about the changes since it is interested */
  d->spellingMistakesModel->projectFilesChanged( added, removed );

  emit projectFilesChanged( added, removed );
}
// --------------------------------------------------

void SpellCheckerCore::mangerEditorChanged( Core::IEditor* editor )
{
  d->currentFilePath.clear();
  d->currentEditor = editor;
  if( editor != nullptr ) {
    d->currentFilePath = editor->document()->filePath().toString();
  }

  emit currentEditorChanged( d->currentFilePath );

  WordList wl;
  if( d->currentFilePath.isEmpty() == false ) {
    wl = d->spellingMistakesModel->mistakesForFile( d->currentFilePath );
  }
  d->mistakesModel->setCurrentSpellingMistakes( wl );
}
// --------------------------------------------------

void SpellCheckerCore::editorOpened( Core::IEditor* editor )
{
  if( editor == nullptr ) {
    return;
  }
  connect( qobject_cast<QPlainTextEdit*>( editor->widget() ), &QPlainTextEdit::cursorPositionChanged, this, &SpellCheckerCore::cursorPositionChanged );
}
// --------------------------------------------------

void SpellCheckerCore::editorAboutToClose( Core::IEditor* editor )
{
  if( editor == nullptr ) {
    return;
  }
  disconnect( qobject_cast<QPlainTextEdit*>( editor->widget() ), &QPlainTextEdit::cursorPositionChanged, this, &SpellCheckerCore::cursorPositionChanged );
}
// --------------------------------------------------

void SpellCheckerCore::updateContextMenu()
{
  if( d->contextMenuHolderCommands.isEmpty() == true ) {
    /* Populate the internal vector with the holder actions to speed up the process
     * of updating the context menu when requested again. */
    QVector<const char*> holderActionIds { Constants::ACTION_HOLDER1_ID, Constants::ACTION_HOLDER2_ID, Constants::ACTION_HOLDER3_ID, Constants::ACTION_HOLDER4_ID, Constants::ACTION_HOLDER5_ID };
    /* Iterate the commands and */
    for( int count = 0; count < holderActionIds.size(); ++count ) {
      Core::Command* cmd = Core::ActionManager::command( holderActionIds[count] );
      d->contextMenuHolderCommands.push_back( cmd );
    }
  }

  Word word;
  bool isMistake = isWordUnderCursorMistake( word );
  /* Do nothing if the context menu is not a mistake.
   * The context menu will in this case already be disabled so there
   * is no need to update it. */
  if( isMistake == false ) {
    return;
  }
  QStringList list = word.suggestions;
  /* Iterate the commands and */
  for( Core::Command* cmd: qAsConst( d->contextMenuHolderCommands ) ) {
    Q_ASSERT( cmd != nullptr );
    if( list.size() > 0 ) {
      /* Disconnect the previous connection made, otherwise it will also trigger */
      cmd->action()->disconnect();
      /* Set the text on the action for the word to use*/
      QString replacementWord = list.takeFirst();
      cmd->action()->setText( replacementWord );
      /* Show the action */
      cmd->action()->setVisible( true );
      /* Connect to lambda function to call to replace the words if the
       * action is triggered. */
      connect( cmd->action(), &QAction::triggered, this, [this, word, replacementWord]() {
        WordList wordsToReplace;
        if( d->settings.replaceAllFromRightClick == true ) {
          this->getAllOccurrencesOfWord( word, wordsToReplace );
        } else {
          wordsToReplace.append( word );
        }
        this->replaceWordsInCurrentEditor( wordsToReplace, replacementWord );
      } );
    } else {
      /* Hide the action since there are less than 5 suggestions for the word. */
      cmd->action()->setVisible( false );
    }
  }
}
// --------------------------------------------------
