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

#include "../../spellcheckerconstants.h"
#include "../../spellcheckercore.h"
#include "../../spellcheckercoresettings.h"
#include "../../Word.h"
#include "cppdocumentparser.h"
#include "cppdocumentprocessor.h"
#include "cppparserconstants.h"
#include "cppparseroptionspage.h"
#include "cppparsersettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditordocument.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cpptoolsreuse.h>
#include <projectexplorer/project.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditor.h>
#include <utils/algorithm.h>
#include <utils/mimeutils.h>
#include <utils/runextensions.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QTextBlock>

/*! \brief Testing assert that should be used during debugging
 * but should not be made part of a release. */
// #define SP_CHECK( test ) QTC_CHECK( test )
#define SP_CHECK( test )

namespace SpellChecker {
namespace CppSpellChecker {
namespace Internal {

/*! Mime Type for the C++ doxygen files.
 * This must match the 'mime-type type' in the json.in file of this
 * plugin. */
const char MIME_TYPE_CXX_DOX[] = "text/x-c++dox";
/*! Task index name for the C++ document parser progress notification. */
const char TASK_INDEX[] = "SpellChecker.Task.CppParse";

// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
/*! \brief Wrapper for HashWords to ensure proper locking.
 *
 * Even after a lot of diligence and effort there were still threading
 * issues with some container. For this reason it was decided to add
 * wrappers around some aspects to try and force proper usage.
 *
 * This should allow for better maintainability by removing the burden of
 * locking from the user completely and placing it on the maintainer of this
 * class. */
class LockedTokenHash
{
  LockedTokenHash( const LockedTokenHash& other )      = delete;
  LockedTokenHash& operator=( const LockedTokenHash& ) = delete;
public:
  /*! \brief Constructor. */
  LockedTokenHash() = default;
  /*! \brief Get a copy of the HashWords. */
  HashWords get() const
  {
    QMutexLocker locker( &d_mutex );
    HashWords words = d_tokenHashes;
    return words;
  }
  /*! \brief Clear the hash words.
   *
   * This function can probably be removed since one can clear
   * the hash using the assignment operator with an empty hash, but
   * for clearness sake the function is kept. */
  void clear()
  {
    QMutexLocker locker( &d_mutex );
    d_tokenHashes.clear();
  }
  /*! \brief Assign a HashWords to the internal hashes. */
  void operator=( const HashWords& other )
  {
    QMutexLocker locker( &d_mutex );
    d_tokenHashes = other;
  }

private:
  HashWords d_tokenHashes; /*!< The HashWords that are protected. */
  mutable QMutex d_mutex;  /*!< The lock that guards the hashes. */
};

/*! \brief The ProgressNotification Wrapper.
 *
 * Even after a lot of diligence and effort there were still threading
 * issues with some container. For this reason it was decided to add
 * wrappers around some aspects to try and force proper usage.
 *
 * This wrapper wraps the QFutureInterface used for progress notifications.
 * The wrapper uses the update() function to create or destroy the future
 * as needed.
 *
 * This wrapper will probably be added to its own header for re-use if
 * notifications gets added to other parts of the plugin. */
class ProgressNotification
{
  ProgressNotification( const ProgressNotification& )            = delete;
  ProgressNotification& operator=( const ProgressNotification& ) = delete;
public:
  /*! \brief Constructor. */
  ProgressNotification() = default;
  /*! \brief Destructor.
   *
   * The destructor will ensure that the future gets cancelled before
   * destroying it. */
  ~ProgressNotification()
  {
    cancel();
  }
  /*! \brief Update the progress indicator.
   *
   * Use the supplied arguments along with the current state of the indicator
   * to update the progress.
   *
   * If there are enough files outstanding and the indicator does not exist it
   * will be created. If there are not enough outstanding and in progress,
   * the indicator will be removed. If the indicator exists, it will be updated.
   * The "enough" is a hard coded, thumb-sucked value...
   *
   * \param filesInProject Total files that must be processed.
   * \param outstanding Number of files that must still be processed.
   * \param inProcess Number of files that are currently in process. */
  void update( int32_t filesInProject, int32_t outstanding, int32_t inProcess )
  {
    /* Thumb-suck value to decide when the future should be created and
     * when it should be destroyed. */
    constexpr int32_t cFILE_OUT_COUNT = 10;
    QMutexLocker locker( &d_mutex );
    /* If there are more than 10 files in the waiting queue, create a progress
     * notification. */
    if( ( outstanding > cFILE_OUT_COUNT )
        && ( d_progressObject == nullptr ) ) {
      d_progressObject = std::make_unique<QFutureInterface<void>>();
      d_progressObject->setProgressRange( 0, filesInProject );
      /* Add the task. The title and type will become constructor arguments
       * when this class gets generalised. */
      Core::ProgressManager::addTask( d_progressObject->future(), CppDocumentParser::tr( "SpellChecker: Parse C/C++ Files" ), TASK_INDEX );
      d_progressObject->reportStarted();
      /* Update the progress here and return immediately.
       * This is done to prevent the check below that should be unnecessary
       * since the object will be valid and should not be deleted. */
      d_progressObject->setProgressValue( filesInProject - outstanding - inProcess );
      return;
    }

    /* If there is a progress notification, update it */
    if( d_progressObject != nullptr ) {
      d_progressObject->setProgressRange( 0, filesInProject );
      d_progressObject->setProgressValue( filesInProject - outstanding - inProcess );
      if( ( outstanding + inProcess ) < cFILE_OUT_COUNT ) {
        /* All done, remove the progress notification. */
        d_progressObject->reportFinished();
        d_progressObject.reset();
      }
    }
  }
  /*! \brief Cancel the notification and destroy the future. */
  void cancel()
  {
    QMutexLocker locker( &d_mutex );
    /* If there is a progress report, cancel it. */
    if( d_progressObject != nullptr ) {
      d_progressObject->reportCanceled();
      d_progressObject.reset();
    }
  }

private:
  using ProgressObjPtr = std::unique_ptr<QFutureInterface<void>>;
  ProgressObjPtr d_progressObject; /*!< The progress object. */
  mutable QMutex d_mutex;          /*!< The lock that guards the future. */
};

/*! \brief List of Future Watchers that are created.
 *
 * Even after a lot of diligence and effort there were still threading
 * issues with some container. For this reason it was decided to add
 * wrappers around some aspects to try and force proper usage.
 *
 * This wrapper wraps a map containing future watchers that watch the
 * processor objects.
 *
 * An added benefit to proper protection is that once can also change the
 * storage type of the watchers without having to change the users. */
class FutureWatchers
{
  /* Using declarations to simplify the code a bit. */
  using FutureWatcher        = CppDocumentProcessor::WatcherPtr;
  using FutureWatcherMap     = QMap<FutureWatcher, QString>;
  using FutureWatcherMapIter = FutureWatcherMap::Iterator;
  /* Prevent copy and assignment */
  FutureWatchers( const FutureWatchers& )            = delete;
  FutureWatchers& operator=( const FutureWatchers& ) = delete;
public:
  /*! \brief Constructor. */
  FutureWatchers() = default;
  /*! \brief Add a new \a watcher and with its \a fileName. */
  void add( CppDocumentProcessor::WatcherPtr watcher, const QString& fileName )
  {
    QMutexLocker locker( &d_mutex );
    d_futureWatchers.insert( watcher, fileName );
  }
  /*! \brief Remove a watcher.
   *
   * This function will remove the watcher if it is in the list of
   * watchers. If it was found, the name of the associated file
   * will be returned. If it was not found, the name will be empty.
   *
   * The reason for returning the name is due to the way that this
   * object is used. This is probably bad design, but good for speed
   * compared to first getting the associated name and then calling
   * remove */
  QString remove( CppDocumentProcessor::WatcherPtr watcher )
  {
    QMutexLocker locker( &d_mutex );
    QString fileName;
    /* Get the file name associated with this future and the misspelled
     * words. */
    FutureWatcherMapIter iter = d_futureWatchers.find( watcher );
    SP_CHECK( iter != d_futureWatchers.end() );
    if( iter != d_futureWatchers.end() ) {
      fileName = iter.value();
      d_futureWatchers.erase( iter );
    }
    return fileName;
  }
  /*! \brief Cancel all futures.
   *
   * This function will block until all futures that were cancelled
   * have finished. The function will also remove all futures from the
   * list of futures. */
  void cancell()
  {
    QMutexLocker locker( &d_mutex );
    for( FutureWatcherMapIter iter = d_futureWatchers.begin(); iter != d_futureWatchers.end(); ++iter ) {
      /* Can't use range for or std::for_each due to the way that a Qt QMap works... */
      iter.key()->cancel();
    }
    for( FutureWatcherMapIter iter = d_futureWatchers.begin(); iter != d_futureWatchers.end(); ++iter ) {
      /* Can't use range for or std::for_each due to the way that a Qt QMap works... */
      iter.key()->waitForFinished();
    }
    d_futureWatchers.clear();
  }
private:
  FutureWatcherMap d_futureWatchers; /*!< Map of watchers that should be guarded. */
  mutable QMutex d_mutex;            /*!< The lock that guards the map. */
};

/*! \brief PIMPL of the CppDocumentParser object. */
class CppDocumentParserPrivate
{
public:
  ProjectExplorer::Project* activeProject;
  QString currentEditorFileName;
  CppParserSettings settings;
  CppParserOptionsPage optionsPage{&settings};
  QStringSet filesInStartupProject;
  // --
  QMutex fileQeueMutex;                /*!< Mutex protecting the filesToUpdate and filesInProcess
                                        * sets. This should also be added to a wrapper, but for
                                        * now this will be skipped. */
  std::set<QString> filesToUpdate;     /*!< Files added to the waiting queue
                                        * that must still be parsed. The
                                        * CppModelManager must still be instructed
                                        * to parse these files. The idea is not to
                                        * instruct too many at a time since this
                                        * can be an issue for large projects.
                                        * A std::set was used to make threading
                                        * issues clear, compared to a QSet with
                                        * COW that hides this (and introduces
                                        * confusion). */
  std::set<QString> filesInProcess;    /*!< Files that are in process of being
                                        * parsed. Either the CppModelManager was
                                        * instructed to parse the file or there is
                                        * already a future parsing the file.
                                        * See above for why a std::set was used. */
  LockedTokenHash tokenHashes;         /*!< Tokens and their hashes that are
                                        * used to speed up processing the
                                        * current file. The hashes of tokens
                                        * (comments, literals, etc.) and their
                                        * words are kept so that if the same token
                                        * is encountered, the words can be reused
                                        * without needing to process the token
                                        * again. */
  FutureWatchers futureWatchers;       /*!< List of future watchers created. This
                                        * list is used to cancel the futures as needed
                                        * for example when the application closes down,
                                        * the project changes or the settings changes. */
  ProgressNotification progressObject; /*!< The object pointer for the
                                        * progress indication. It will get
                                        * created and destroyed as needed
                                        * by the parser. */

  CppDocumentParserPrivate()
    : activeProject( nullptr )
    , currentEditorFileName()
    , filesInStartupProject()
    , progressObject()
  {}

  /*! \brief Get all C++ files from the \a list of files.
   *
   * This function uses the MIME Types of the passed files and check if
   * they are classified by the CppTools::ProjectFile class. If they are
   * they are regarded as C++ files.
   * If a file is unsupported the type is checked against the
   * custom MIME Type added by this plugin. */
  QStringSet getCppFiles( const QStringSet& list )
  {
    return Utils::filtered( list, []( const QString& file ) {
            const CppEditor::ProjectFile::Kind kind = CppEditor::ProjectFile::classify( file );
            switch( kind ) {
              case CppEditor::ProjectFile::Unclassified:
                return false;
              case CppEditor::ProjectFile::Unsupported: {
                /* Check our doxy MimeType added by this plugin */
                const Utils::MimeType mimeType = Utils::mimeTypeForFile( file );
                const QString mt               = mimeType.name();
                if( mt == QLatin1String( MIME_TYPE_CXX_DOX ) ) {
                  return true;
                } else {
                  return false;
                }
              }
              default:
                return true;
            }
          } );
  }
  // ------------------------------------------

  /*! \brief Utility function to erase a file from the set if the file
   * is in the set.
   *
   * Since std::set::erase() requires a valid iterator (not end(), see
   * the docs) a check must first be done to check if the \a file was
   * found or now. */
  static void eraseIfFound( std::set<QString>& set, const QString& file )
  {
    const auto setIter = set.find( file );
    if( setIter != set.end() ) {
      set.erase( setIter );
    }
  }
  // ------------------------------------------
};
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

CppDocumentParser::CppDocumentParser( QObject* parent )
  : IDocumentParser( parent )
  , d( new CppDocumentParserPrivate() )
{
  /* Create the settings for this parser */
  d->settings.loadFromSettings( Core::ICore::settings() );
  connect(                &d->settings,               &CppParserSettings::settingsChanged,                                this, &CppDocumentParser::settingsChanged );
  connect( SpellCheckerCore::instance()->settings(), &SpellChecker::Internal::SpellCheckerCoreSettings::settingsChanged, this, &CppDocumentParser::settingsChanged );

  CppEditor::CppModelManager* modelManager = CppEditor::CppModelManager::instance();
  connect( modelManager, &CppEditor::CppModelManager::documentUpdated, this, &CppDocumentParser::parseCppDocumentOnUpdate, Qt::DirectConnection );
  connect(         qApp, &QApplication::aboutToQuit,                  this, [=]() {
          /* Disconnect any signals that might still get emitted. */
          modelManager->disconnect( this );
          SpellCheckerCore::instance()->disconnect( this );
          this->disconnect( SpellCheckerCore::instance() );
        }, Qt::DirectConnection );

  Core::Context context( CppEditor::Constants::CPPEDITOR_ID );
  Core::ActionContainer* cppEditorContextMenu = Core::ActionManager::createMenu( CppEditor::Constants::M_CONTEXT );
  Core::ActionContainer* contextMenu          = Core::ActionManager::createMenu( Constants::CONTEXT_MENU_ID );
  cppEditorContextMenu->addSeparator( context );
  cppEditorContextMenu->addMenu( contextMenu );
  connect( qApp, &QCoreApplication::aboutToQuit, this, &CppDocumentParser::aboutToQuit, Qt::DirectConnection );

  connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
          this, [this] { d->settings.saveToSetting(Core::ICore::settings()); });
}
// --------------------------------------------------

CppDocumentParser::~CppDocumentParser()
{
  delete d;
}
// --------------------------------------------------

QString CppDocumentParser::displayName()
{
  return tr( "C++ Document Parser" );
}
// --------------------------------------------------

Core::IOptionsPage* CppDocumentParser::optionsPage()
{
  //https://github.com/qt-creator/qt-creator/commit/9a42382fd15fd9d2f5a94737f6d496478a6bf096#diff-4fca580adee50ff91282c1701ecc68f8a59cd47b86a2a30adea522826409e2ec
  return &d->optionsPage;
}
// --------------------------------------------------

void CppDocumentParser::setActiveProject( ProjectExplorer::Project* activeProject )
{
  d->activeProject = activeProject;
  d->filesInStartupProject.clear();

  /* Call reparseProject() to reset and clean up properly.
   * The logic inside will ensure that parsing is not started again
   * since there is no active project. */
  reparseProject();
}
// --------------------------------------------------

void CppDocumentParser::updateProjectFiles( QStringSet filesAdded, QStringSet filesRemoved )
{
  Q_UNUSED( filesRemoved )
  const QStringSet fileSet = d->getCppFiles( filesAdded );
  d->filesInStartupProject.unite( fileSet );
  {
    QMutexLocker locker( &d->fileQeueMutex );
    d->filesToUpdate.insert( fileSet.cbegin(), fileSet.cend() );
  }
  queueFilesForUpdate();
}
// --------------------------------------------------

void CppDocumentParser::setCurrentEditor( const QString& editorFilePath )
{
  d->currentEditorFileName = editorFilePath;
}
// --------------------------------------------------

void CppDocumentParser::parseCppDocumentOnUpdate( CPlusPlus::Document::Ptr docPtr )
{
  if( docPtr.isNull() == true ) {
    return;
  }

  const QString fileName = docPtr->filePath().toString();
  const bool shouldParse = shouldParseDocument( fileName );

  bool queueMore;
  {
    QMutexLocker locker( &d->fileQeueMutex );
    /* Remove from the list to update since it will be updated now */
    d->eraseIfFound( d->filesToUpdate, fileName );
    /* Always try to queue more if there are more files to update.
     * The logic inside queueFilesForUpdate() will ensure that there
     * are no more added than what is desired. */
    queueMore = ( d->filesToUpdate.size() > 0 );

    /* If the file should not be parsed, remove it from the list of
     * files in process. This is needed since the queueFilesForUpdate()
     * will add it to that list when it queues it.
     * If the file should be parsed, add it to the list of files that
     * that are in process, this is used to limit the number of files
     * processed at the same time. */
    if( shouldParse == false ) {
      d->eraseIfFound( d->filesInProcess, fileName );
    } else {
      d->filesInProcess.insert( fileName );
    }
  }

  if( shouldParse == true ) {
    parseCppDocument( std::move( docPtr ) );
  }

  if( queueMore == true ) {
    queueFilesForUpdate();
  }
}
// --------------------------------------------------

void CppDocumentParser::settingsChanged()
{
  /* Clear the hashes since all comments must be re parsed. */
  d->tokenHashes.clear();
  /* Re parse the project */
  reparseProject();
}
// --------------------------------------------------

void CppDocumentParser::reparseProject()
{
  /* Need to cancel all futures in process.
   * This function call will block until all are cancelled and done. */
  d->futureWatchers.cancell();
  /* Clear other members. */
  d->filesInStartupProject.clear();
  d->progressObject.cancel();

  /* No active project, do nothing */
  if( d->activeProject == nullptr ) {
    return;
  }

  const Utils::FilePaths projectFiles = d->activeProject->files( ProjectExplorer::Project::SourceFiles );
  const QStringList fileList             = Utils::transform( projectFiles, &Utils::FilePath::toString );

  const QStringSet fileSet = d->getCppFiles( QStringSet(fileList.begin(), fileList.end()) );
  d->filesInStartupProject = fileSet;

  {
    /* Add the files to the waiting queue and then process the queue */
    QMutexLocker locker( &d->fileQeueMutex );
    d->filesInProcess.clear();
    d->filesToUpdate.clear();
    d->filesToUpdate = Utils::transform<std::set<QString>>( fileSet, []( const QString& string ) { return string; } );
  }

  queueFilesForUpdate();
}
// --------------------------------------------------

void CppDocumentParser::queueFilesForUpdate()
{
  /* Only re-parse the files that were added. */
  static CppEditor::CppModelManager* modelManager = CppEditor::CppModelManager::instance();

  QSet<Utils::FilePath> filesToUpdate;
  size_t filesOutstanding;
  size_t filesInProcess;

  {
    QMutexLocker locker( &d->fileQeueMutex );
    auto fileIter = d->filesToUpdate.begin();
    while( ( d->filesInProcess.size() < 10 )
           && ( d->filesToUpdate.empty() == false ) ) {
      const QString file = ( *fileIter );
      fileIter = d->filesToUpdate.erase( fileIter );
      if( shouldParseDocument( file ) == true ) {
        d->filesInProcess.insert( file );
        filesToUpdate.insert( Utils::FilePath::fromString(file) );
      }
    }

    filesOutstanding = d->filesToUpdate.size();
    filesInProcess   = d->filesInProcess.size();
  }

  d->progressObject.update( d->filesInStartupProject.count(), int32_t( filesOutstanding ), int32_t( filesInProcess ) );

  modelManager->updateSourceFiles( filesToUpdate );
}
// --------------------------------------------------

bool CppDocumentParser::shouldParseDocument( const QString& fileName )
{
  SpellChecker::Internal::SpellCheckerCoreSettings* settings = SpellCheckerCore::instance()->settings();
  if( ( settings->onlyParseCurrentFile == true )
      && ( d->currentEditorFileName != fileName ) ) {
    /* The global setting is set to only parse the current file and the
    * file asked about is not the current one, thus do not parse it. */
    return false;
  }

  if( ( settings->checkExternalFiles ) == false ) {
    /* Do not check external files so check if the file is part of the
     * active project. */
    return d->filesInStartupProject.contains( fileName );
  }

  return true;
}
// --------------------------------------------------

void CppDocumentParser::futureFinished()
{
  /* Get the watcher from the sender() of the signal that invoked this slot.
   * reinterpret_cast is used since qobject_cast is not valid of template
   * classes since the template class does not have the Q_OBJECT macro. */
  auto watcher = reinterpret_cast<CppDocumentProcessor::WatcherPtr>( sender() );
  SP_CHECK( watcher != nullptr );
  if( watcher->isCanceled() == true ) {
    /* Application is shutting down or settings changed etc.
     * If a watcher was cancelled, it would already be removed out of
     * the list of watchers, thus no need to do anything more. */
    return;
  }
  const CppDocumentProcessor::ResultType result = watcher->result();

  const QString fileName = d->futureWatchers.remove( watcher );
  if( fileName == d->currentEditorFileName ) {
    /* Move the new list of hashes to the member data so that
     * it can be used the next time around. Move is made explicit since
     * the LHS can be removed and the RHS will not be used again from
     * here on. */
    d->tokenHashes = std::move( result.wordHashes );
  }

  {
    QMutexLocker locker( &d->fileQeueMutex );
    d->eraseIfFound( d->filesInProcess, fileName );
  }
  queueFilesForUpdate();

  /* Now that we have all of the words from the parser, emit the signal
   * so that they will get spell checked. */
  emit spellcheckWordsParsed( fileName, result.words );
}
// --------------------------------------------------

void CppDocumentParser::aboutToQuit()
{
  setActiveProject( nullptr );
}
// --------------------------------------------------

void CppDocumentParser::parseCppDocument( CPlusPlus::Document::Ptr docPtr )
{
  using Watcher    = CppDocumentProcessor::Watcher;
  using WatcherPtr = CppDocumentProcessor::WatcherPtr;
  using ResultType = CppDocumentProcessor::ResultType;
  const QString fileName = docPtr->filePath().toString();
  HashWords hashes;
  if( fileName == d->currentEditorFileName ) {
    hashes = d->tokenHashes.get();
  }
  /* Create a document parser and move it to the main thread.
   * Not sure if this is required but it seemed like a good
   * idea since this will be in a QThreadPool thread. */
  CppDocumentProcessor* parser = new CppDocumentProcessor( docPtr, hashes, d->settings );
  parser->moveToThread( qApp->thread() );
  /* Reset the document pointer so that it can be released as soon as it is
   * done in the processor. The processor makes its own copy to keep it
   * alive. */
  docPtr.reset();

  /* Create a Future watcher that will be used to watch the future
   * promised by the processor.
   * NB: The moveToThread() call is vital. If the future is not in the
   * main thread, the finished() signals will not be delivered and the
   * processing does not work. */
  WatcherPtr watcher = new Watcher();
  watcher->moveToThread( qApp->thread() );
  connect( watcher, &Watcher::finished, this,   &CppDocumentParser::futureFinished, Qt::QueuedConnection );
  connect( watcher, &Watcher::finished, parser, &CppDocumentProcessor::deleteLater );
  /* Keep track of the watchers so that they can be cancelled as needed. */
  d->futureWatchers.add( watcher, fileName );
  /* Create a future to process the file.
   * If the file to process is the current open editor, it is parsed in a new
   * thread with high priority.
   * If it is not the current file, it is added to the global thread pool
   * since it can get processed in its own time.
   * The current one gets a new thread so that it can get processed as
   * soon as possible and it does not need to get queued along with all other
   * futures added to the global thread pool. */
  if( fileName == d->currentEditorFileName ) {
    QFuture<ResultType> future = Utils::runAsync( QThread::HighPriority, &CppDocumentProcessor::process, parser );
    watcher->setFuture( future );
  } else {
    QFuture<ResultType> future = Utils::runAsync( QThreadPool::globalInstance(), QThread::NormalPriority, &CppDocumentProcessor::process, parser );
    watcher->setFuture( future );
  }
}
// --------------------------------------------------

void CppDocumentParser::applySettingsToWords( const CppParserSettings& settings, const QString& string, const QStringSet& wordsInSource, WordList& words )
{
  using namespace SpellChecker::Parsers::CppParser;

  /* Filter out words that appears in the source. They are checked against the list
   * of words parsed from the file before the for loop. */
  if( settings.removeWordsThatAppearInSource == true ) {
    removeWordsThatAppearInSource( wordsInSource, words );
  }

  /* Regular Expressions that might be used, defined here so that it does not get cleared in the
   * loop. They are made static const because they will be re-used a lot and will never be changed.
   * This way the construction of the objects can be done once and then be re-used. */
  static const QRegularExpression doubleRe( QStringLiteral( "\\A\\d+(\\.\\d+)?\\z" ), QRegularExpression::DontCaptureOption );
  static const QRegularExpression hexRe( QStringLiteral( "\\A0x[0-9A-Fa-f]+\\z" ) );
  static const QRegularExpression colorRe( QStringLiteral( "\\A([0-9A-Fa-f]{2}){3,4}\\z" ), QRegularExpression::DontCaptureOption );
  static const QRegularExpression emailRe( QStringLiteral( "\\A" ) + QLatin1String( SpellChecker::Parsers::CppParser::Constants::EMAIL_ADDRESS_REGEXP_PATTERN ) + QStringLiteral( "\\z" ) );
  static const QRegularExpression websiteRe( QString() + QLatin1String( SpellChecker::Parsers::CppParser::Constants::WEBSITE_ADDRESS_REGEXP_PATTERN ) );
  static const QRegularExpression websiteCharsRe( QString() + QLatin1String( SpellChecker::Parsers::CppParser::Constants::WEBSITE_CHARS_REGEXP_PATTERN ) );

  /* Word list that can be added to in the case that a word is split up into different words
   * due to some setting or rule. These words can also be checked against the settings using
   * recursion or not. It depends on the implementation that did the splitting of the
   * original word. It is done in this way so that the iterator that is currently operating
   * on the list of words does not break when new words get added during iteration */
  WordList wordsToAddInTheEnd;
  /* Iterate through the list of words using an iterator and remove words according to settings */
  WordList::Iterator iter = words.begin();
  while( iter != words.end() ) {
    const Word& word        = ( *iter );
    QString currentWord     = word.text;
    QString currentWordCaps = currentWord.toUpper();
    bool removeCurrentWord  = false;

    /* Remove reserved words first. Although this does not depend on settings, this
     * is done here to prevent multiple iterations through the word list where possible */
    removeCurrentWord = isReservedWord( currentWord );

    if( removeCurrentWord == false ) {
      /* Remove the word if it is a number, checking for floats and doubles as well.
       * Or if it is a hex number
       * Or if it can be a color and it starts with a #, then it is a color.*/
      removeCurrentWord = ( doubleRe.match( currentWord ).hasMatch() == true )
                          || ( hexRe.match( currentWord ).hasMatch() == true )
                          || ( ( colorRe.match( currentWord ).hasMatch() == true )
                               && ( string.at( word.start - 1 ) == QLatin1Char( '#' ) ) );

    }

    if( ( removeCurrentWord == false ) && ( settings.checkQtKeywords == false ) ) {
      /* Remove the basic Qt Keywords using the isQtKeyword() function in the CppTools */
      if( ( CppEditor::isQtKeyword( currentWord ) == true )
          || ( CppEditor::isQtKeyword( currentWordCaps ) == true ) ) {
        removeCurrentWord = true;
      }
      /* Remove words that Start with capital Q and the next char is also capital letter. This would
       * only apply to words longer than 2 characters long. This check is also to ensure that we do
       * not go past the size of the word */
      if( currentWord.length() > 2 ) {
        if( ( currentWord.at( 0 ) == QLatin1Char( 'Q' ) ) && ( currentWord.at( 1 ).isUpper() == true ) ) {
          removeCurrentWord = true;
        }
      }

      /* Remove all caps words that start with Q_ */
      if( currentWord.startsWith( QLatin1String( "Q_" ), Qt::CaseSensitive ) == true ) {
        removeCurrentWord = true;
      }

      /* Remove qDebug() */
      if( currentWord == QLatin1String( "qDebug" ) ) {
        removeCurrentWord = true;
      }
    }

    if( ( settings.removeEmailAddresses == true ) && ( removeCurrentWord == false ) ) {
      if( emailRe.match( currentWord ).hasMatch() == true ) {
        removeCurrentWord = true;
      }
    }

    /* Attempt to remove website addresses using the websiteRe Regular Expression. */
    if( ( settings.removeWebsites == true ) && ( removeCurrentWord == false ) ) {
      if( websiteRe.match( currentWord ).hasMatch() == true ) {
        removeCurrentWord = true;
      } else if( currentWord.contains( websiteCharsRe ) == true ) {
        QStringList wordsSplitOnWebChars = currentWord.split( websiteCharsRe, Qt::SkipEmptyParts );
        if( wordsSplitOnWebChars.isEmpty() == false ) {
          /* String is not a website, check each component now */
          removeCurrentWord = true;
          WordList wordsFromSplit;
          IDocumentParser::getWordsFromSplitString( wordsSplitOnWebChars, word, wordsFromSplit );
          /* Apply the settings to the words that came from the split to filter out words that does
           * not belong due to settings. After they have passed the settings, add the words that
           * survived to the list of words that should be added in the end */
          applySettingsToWords( settings, string, wordsInSource, wordsFromSplit );
          wordsToAddInTheEnd.append( wordsFromSplit );
        }
      }
    }

    if( ( settings.checkAllCapsWords == false ) && ( removeCurrentWord == false ) ) {
      /* Remove words that are all caps */
      if( currentWord == currentWordCaps ) {
        removeCurrentWord = true;
      }
    }

    if( ( settings.wordsWithNumberOption != CppParserSettings::LeaveWordsWithNumbers ) && ( removeCurrentWord == false ) ) {
      /* Before doing anything, check if the word contains any numbers. If it does then we can go to
       * the settings to handle the word differently */
      static const QRegularExpression numberContainRe( QStringLiteral( "[0-9]" ) );
      static const QRegularExpression numberSplitRe( QStringLiteral( "[0-9]+" ) );
      if( currentWord.contains( numberContainRe ) == true ) {
        /* Handle words with numbers based on the setting that is set for them */
        if( settings.wordsWithNumberOption == CppParserSettings::RemoveWordsWithNumbers ) {
          removeCurrentWord = true;
        } else if( settings.wordsWithNumberOption == CppParserSettings::SplitWordsOnNumbers ) {
          removeCurrentWord = true;
          QStringList wordsSplitOnNumbers = currentWord.split( numberSplitRe, Qt::SkipEmptyParts );
          WordList wordsFromSplit;
          IDocumentParser::getWordsFromSplitString( wordsSplitOnNumbers, word, wordsFromSplit );
          /* Apply the settings to the words that came from the split to filter out words that does
           * not belong due to settings. After they have passed the settings, add the words that
           * survived to the list of words that should be added in the end */
          applySettingsToWords( settings, string, wordsInSource, wordsFromSplit );
          wordsToAddInTheEnd.append( wordsFromSplit );
        } else {
          /* Should never get here */
          QTC_CHECK( false );
        }
      }
    }

    if( ( settings.wordsWithUnderscoresOption != CppParserSettings::LeaveWordsWithUnderscores ) && ( removeCurrentWord == false ) ) {
      /* Check to see if the word has underscores in it. If it does then handle according to the
       * settings */
      if( currentWord.contains( QLatin1Char( '_' ) ) == true ) {
        if( settings.wordsWithUnderscoresOption == CppParserSettings::RemoveWordsWithUnderscores ) {
          removeCurrentWord = true;
        } else if( settings.wordsWithUnderscoresOption == CppParserSettings::SplitWordsOnUnderscores ) {
          removeCurrentWord = true;
          static const QRegularExpression underscoreSplitRe( QStringLiteral( "_+" ) );
          QStringList wordsSplitOnUnderScores = currentWord.split( underscoreSplitRe, Qt::SkipEmptyParts );
          WordList wordsFromSplit;
          IDocumentParser::getWordsFromSplitString( wordsSplitOnUnderScores, word, wordsFromSplit );
          /* Apply the settings to the words that came from the split to filter out words that does
           * not belong due to settings. After they have passed the settings, add the words that
           * survived to the list of words that should be added in the end */
          applySettingsToWords( settings, string, wordsInSource, wordsFromSplit );
          wordsToAddInTheEnd.append( wordsFromSplit );
        } else {
          /* Should never get here */
          QTC_CHECK( false );
        }
      }
    }

    /* Settings for CamelCase */
    if( ( settings.camelCaseWordOption != CppParserSettings::LeaveWordsInCamelCase ) && ( removeCurrentWord == false ) ) {
      /* Check to see if the word appears to be in camelCase. If it does, handle according to the
       * settings */
      /* The check is not precise and accurate science, but a rough estimation of the word is in
       * camelCase. This will probably be updated as this gets tested. The current check checks for
       * one or more lower case letters,
       * followed by one or more upper-case letter, followed by a lower case letter */
      static const QRegularExpression camelCaseContainsRe( QStringLiteral( "[a-z]{1,}[A-Z]{1,}[a-z]{1,}" ) );
      static const QRegularExpression camelCaseIndexRe( QStringLiteral( "[a-z][A-Z]" ) );
      if( currentWord.contains( camelCaseContainsRe ) == true ) {
        if( settings.camelCaseWordOption == CppParserSettings::RemoveWordsInCamelCase ) {
          removeCurrentWord = true;
        } else if( settings.camelCaseWordOption == CppParserSettings::SplitWordsOnCamelCase ) {
          removeCurrentWord = true;
          QStringList wordsSplitOnCamelCase;
          /* Search the word for all indexes where there is a lower case letter followed by an upper
           * case letter. This indexes are then later used to split the current word into a list of
           * new words. 0 is added as the starting index, since the first word will start at 0. At
           * the end the length
           * of the word is also added, since the last word will stop at the end */
          QList<int> indexes;
          indexes << 0;
          int currentIdx = 0;
          int lastIdx    = 0;
          bool finished  = false;
          while( finished == false ) {
            currentIdx = currentWord.indexOf( camelCaseIndexRe, lastIdx );
            if( currentIdx == -1 ) {
              finished = true;
              indexes << currentWord.length();
            } else {
              lastIdx = currentIdx + 1;
              indexes << lastIdx;
            }
          }
          /* Now split the word on the indexes */
          for( int idx = 0; idx < indexes.count() - 1; ++idx ) {
            /* Get the word starting at the current index, up to the difference between the
             * different index and the current index, since the second argument of QString::mid() is
             * the length to extract and not the index of the last position */
            QString word = currentWord.mid( indexes.at( idx ), indexes.at( idx + 1 ) - indexes.at( idx ) );
            wordsSplitOnCamelCase << word;
          }
          WordList wordsFromSplit;
          /* Get the proper word structures for the words extracted during the split */
          IDocumentParser::getWordsFromSplitString( wordsSplitOnCamelCase, word, wordsFromSplit );
          /* Apply the settings to the words that came from the split to filter out words that does
           * not belong due to settings. After they have passed the settings, add the words that
           * survived to the list of words that should be added in the end */
          applySettingsToWords( settings, string, wordsInSource, wordsFromSplit );
          wordsToAddInTheEnd.append( wordsFromSplit );
        } else {
          /* Should never get here */
          QTC_CHECK( false );
        }
      }
    }

    /* Words.with.dots */
    if( ( settings.wordsWithDotsOption != CppParserSettings::LeaveWordsWithDots ) && ( removeCurrentWord == false ) ) {
      /* Check to see if the word has dots in it.
       * If it does then handle according to the settings */
      if( currentWord.contains( QLatin1Char( '.' ) ) == true ) {
        if( settings.wordsWithDotsOption == CppParserSettings::RemoveWordsWithDots ) {
          removeCurrentWord = true;
        } else if( settings.wordsWithDotsOption == CppParserSettings::SplitWordsOnDots ) {
          removeCurrentWord = true;
          static const QRegularExpression dotsSplitRe( QStringLiteral( "\\.+" ) );
          QStringList wordsSplitOnDots = currentWord.split( dotsSplitRe, Qt::SkipEmptyParts );
          WordList wordsFromSplit;
          IDocumentParser::getWordsFromSplitString( wordsSplitOnDots, word, wordsFromSplit );
          /* Apply the settings to the words that came from the split to filter out words that does
           * not belong due to settings. After they have passed the settings, add the words that
           * survived to the list of words that should be added in the end */
          applySettingsToWords( settings, string, wordsInSource, wordsFromSplit );
          wordsToAddInTheEnd.append( wordsFromSplit );
        } else {
          /* Should never get here */
          QTC_CHECK( false );
        }
      }
    }

    /* Remove the current word if it should be removed. The word will get removed in place. The
     * erase() function on the list will return an iterator to the next element. In this case,
     * the iterator should not be incremented and the while loop should continue to the next
     * element. */
    if( removeCurrentWord == true ) {
      iter = words.erase( iter );
    } else {
      ++iter;
    }
  }
  /* Add the words that should be added in the end to the list of words */
  words.append( wordsToAddInTheEnd );
}
// --------------------------------------------------

// --------------------------------------------------

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker
