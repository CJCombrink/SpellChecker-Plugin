/**************************************************************************
**
** Copyright (c) 2026 Marcel Petrick
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

#include "qmldocumentparser.h"

#include "../../spellcheckerconstants.h"
#include "../../spellcheckercore.h"
#include "../../spellcheckercoresettings.h"
#include "qmldocumentprocessor.h"
#include "qmlparseroptionspage.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <projectexplorer/project.h>
#include <texteditor/textdocument.h>
#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/mimeutils.h>

#include <QApplication>
#include <QFile>
#include <QFutureWatcher>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QThread>
#include <QThreadPool>
#include <QTimer>

using namespace SpellChecker;
using namespace SpellChecker::QmlSpellChecker::Internal;

namespace {

/*! \brief Query whether a file should be treated as QML.
 * \param[in] fileName File path to classify.
 * \return true for QML mime type or `.qml` suffix. */
bool isQmlFile( const QString& fileName )
{
  const Utils::MimeType mimeType = Utils::mimeTypeForFile( fileName );
  const QString mimeName = mimeType.name();
  return mimeName == QLatin1String( "text/x-qml" )
         || fileName.endsWith( QLatin1String( ".qml" ), Qt::CaseInsensitive );
}

/*! \brief Read QML source text from an open editor or from disk.
 *
 * Open text documents are preferred so unsaved editor changes can be parsed.
 * If the file is not open in Qt Creator, the content is read from disk.
 * \param[in] fileName File path to read.
 * \return Source text, or an empty string if the file cannot be read.
 */
QString readSource( const QString& fileName )
{
  TextEditor::TextDocument* textDocument = TextEditor::TextDocument::textDocumentForFilePath( Utils::FilePath::fromString( fileName ) );
  if( textDocument != nullptr ) {
    return textDocument->plainText();
  }

  QFile file( fileName );
  if( file.open( QFile::ReadOnly | QFile::Text ) == false ) {
    return {};
  }

  QTextStream stream( &file );
  return stream.readAll();
}

} // namespace

class SpellChecker::QmlSpellChecker::Internal::QmlDocumentParserPrivate
{
public:
  ProjectExplorer::Project* activeProject = nullptr; /*!< Active project used for project-wide parsing. */
  QString currentEditorFileName;                     /*!< Current editor file path. */
  QmlParserSettings settings;                        /*!< User-visible QML parser settings. */
  QmlParserOptionsPage optionsPage{&settings};       /*!< Options page backed by \a settings. */
  QStringSet filesInStartupProject;                  /*!< QML files that belong to the startup project. */
  QMutex futureMutex;                                /*!< Guards \a futureWatchers. */
  QMap<QmlDocumentProcessor::WatcherPtr, QString> futureWatchers; /*!< Running parse jobs mapped to file paths. */
  QHash<Core::IDocument*, QMetaObject::Connection> documentConnections; /*!< Content-change connections for open QML files. */
  QHash<QString, QTimer*> reparseTimers;             /*!< Debounce timers for live document reparsing. */
};

QmlDocumentParser::QmlDocumentParser( QObject* parent )
  : IDocumentParser( parent )
  , d( new QmlDocumentParserPrivate() )
{
  d->settings.loadFromSettings( Core::ICore::settings() );

  connect( &d->settings, &QmlParserSettings::settingsChanged, this, &QmlDocumentParser::settingsChanged );
  connect( SpellCheckerCore::instance()->settings(), &SpellChecker::Internal::SpellCheckerCoreSettings::settingsChanged, this, &QmlDocumentParser::settingsChanged );

  Core::Context context( QmlJSEditor::Constants::C_QMLJSEDITOR_ID );
  Core::ActionContainer* qmlEditorContextMenu = Core::ActionManager::createMenu( QmlJSEditor::Constants::M_CONTEXT );
  Core::ActionContainer* contextMenu = Core::ActionManager::createMenu( SpellChecker::Constants::CONTEXT_MENU_ID );
  /* Add the shared Spell Check submenu to the QML editor context menu just like
   * the C++ parser does for the C++ editor. */
  qmlEditorContextMenu->addSeparator( context );
  qmlEditorContextMenu->addMenu( contextMenu );

  connect( Core::EditorManager::instance(), &Core::EditorManager::documentOpened, this, &QmlDocumentParser::watchDocument );
  connect( Core::EditorManager::instance(), &Core::EditorManager::documentClosed, this, &QmlDocumentParser::unwatchDocument );
  connect( Core::EditorManager::instance(), &Core::EditorManager::saved, this, [this]( Core::IDocument* document, Core::IDocument::SaveOption ) {
    if( document != nullptr ) {
      parseFile( document->filePath().path() );
    }
  } );
  connect( qApp, &QCoreApplication::aboutToQuit, this, &QmlDocumentParser::aboutToQuit, Qt::DirectConnection );
  connect( Core::ICore::instance(), &Core::ICore::saveSettingsRequested, this, [this] {
    d->settings.saveToSettings( Core::ICore::settings() );
  } );
}

QmlDocumentParser::~QmlDocumentParser()
{
  for( QTimer* timer: std::as_const( d->reparseTimers ) ) {
    timer->stop();
    timer->deleteLater();
  }
  delete d;
}

QString QmlDocumentParser::displayName()
{
  return tr( "QML Document Parser" );
}

Core::IOptionsPage* QmlDocumentParser::optionsPage()
{
  return &d->optionsPage;
}

void QmlDocumentParser::setCurrentEditor( const QString& editorFilePath )
{
  d->currentEditorFileName = editorFilePath;
  parseFile( editorFilePath );
}

void QmlDocumentParser::setActiveProject( ProjectExplorer::Project* activeProject )
{
  d->activeProject = activeProject;
  reparseProject();
}

void QmlDocumentParser::updateProjectFiles( QStringSet filesAdded, QStringSet filesRemoved )
{
  for( const QString& fileName: filesRemoved ) {
    d->filesInStartupProject.remove( fileName );
  }

  const QStringSet qmlFiles = Utils::filtered( filesAdded, []( const QString& fileName ) {
    return isQmlFile( fileName );
  } );

  d->filesInStartupProject.unite( qmlFiles );
  for( const QString& fileName: qmlFiles ) {
    parseFile( fileName );
  }
}

void QmlDocumentParser::settingsChanged()
{
  reparseProject();
  parseFile( d->currentEditorFileName );
}

void QmlDocumentParser::reparseProject()
{
  {
    QMutexLocker locker( &d->futureMutex );
    /* Settings or project changes make outstanding parse results stale. Cancel
     * them before starting a new project parse. */
    for( auto iter = d->futureWatchers.begin(); iter != d->futureWatchers.end(); ++iter ) {
      iter.key()->cancel();
    }
    for( auto iter = d->futureWatchers.begin(); iter != d->futureWatchers.end(); ++iter ) {
      iter.key()->disconnect( this );
      iter.key()->waitForFinished();
      delete iter.key();
    }
    d->futureWatchers.clear();
  }

  d->filesInStartupProject.clear();
  if( d->activeProject == nullptr ) {
    return;
  }

  const Utils::FilePaths projectFiles = d->activeProject->files( ProjectExplorer::Project::SourceFiles );
  const auto fileList = Utils::transform<QStringSet>( projectFiles, &Utils::FilePath::path );
  d->filesInStartupProject = Utils::filtered( fileList, []( const QString& fileName ) {
    return isQmlFile( fileName );
  } );

  for( const QString& fileName: d->filesInStartupProject ) {
    parseFile( fileName );
  }
}

void QmlDocumentParser::parseFile( const QString& fileName )
{
  if( fileName.isEmpty() == true ) {
    return;
  }
  if( isQmlFile( fileName ) == false ) {
    return;
  }
  if( shouldParseDocument( fileName ) == false ) {
    return;
  }

  {
    QMutexLocker locker( &d->futureMutex );
    QList<QmlDocumentProcessor::WatcherPtr> staleWatchers;
    /* Prefer the latest source text for a file. If a previous parse is still
     * running, cancel it so stale results cannot overwrite newer ones. */
    for( auto iter = d->futureWatchers.begin(); iter != d->futureWatchers.end(); ++iter ) {
      if( iter.value() == fileName ) {
        staleWatchers.append( iter.key() );
      }
    }
    for( QmlDocumentProcessor::WatcherPtr watcher: staleWatchers ) {
      d->futureWatchers.remove( watcher );
      watcher->disconnect( this );
      watcher->cancel();
      watcher->waitForFinished();
      delete watcher;
    }
  }

  const QString source = readSource( fileName );
  if( source.isEmpty() == true ) {
    emit spellcheckWordsParsed( fileName, WordList() );
    return;
  }

  auto processor = new QmlDocumentProcessor( fileName, source, d->settings );
  processor->moveToThread( qApp->thread() );

  auto watcher = new QmlDocumentProcessor::Watcher();
  watcher->moveToThread( qApp->thread() );
  connect( watcher, &QmlDocumentProcessor::Watcher::finished, this, &QmlDocumentParser::futureFinished, Qt::QueuedConnection );
  connect( watcher, &QmlDocumentProcessor::Watcher::finished, processor, &QmlDocumentProcessor::deleteLater );

  {
    QMutexLocker locker( &d->futureMutex );
    d->futureWatchers.insert( watcher, fileName );
  }

  if( fileName == d->currentEditorFileName ) {
    watcher->setFuture( Utils::asyncRun( QThread::HighPriority, &QmlDocumentProcessor::process, processor ) );
  } else {
    watcher->setFuture( Utils::asyncRun( QThreadPool::globalInstance(), QThread::LowPriority, &QmlDocumentProcessor::process, processor ) );
  }
}

void QmlDocumentParser::scheduleParseFile( const QString& fileName )
{
  if( ( fileName.isEmpty() == true ) || ( isQmlFile( fileName ) == false ) ) {
    return;
  }

  QTimer* timer = d->reparseTimers.value( fileName, nullptr );
  if( timer == nullptr ) {
    timer = new QTimer( this );
    timer->setSingleShot( true );
    timer->setInterval( 350 );
    d->reparseTimers.insert( fileName, timer );
    connect( timer, &QTimer::timeout, this, [this, fileName] {
      parseFile( fileName );
    } );
  }
  timer->start();
}

void QmlDocumentParser::watchDocument( Core::IDocument* document )
{
  if( document == nullptr ) {
    return;
  }
  const QString fileName = document->filePath().path();
  if( isQmlFile( fileName ) == false ) {
    return;
  }
  if( d->documentConnections.contains( document ) == true ) {
    return;
  }

  d->documentConnections.insert( document, connect( document, &Core::IDocument::contentsChanged, this, [this, document] {
    scheduleParseFile( document->filePath().path() );
  } ) );
  scheduleParseFile( fileName );
}

void QmlDocumentParser::unwatchDocument( Core::IDocument* document )
{
  if( document == nullptr ) {
    return;
  }
  const auto iter = d->documentConnections.find( document );
  if( iter != d->documentConnections.end() ) {
    disconnect( iter.value() );
    d->documentConnections.erase( iter );
  }
}

bool QmlDocumentParser::shouldParseDocument( const QString& fileName ) const
{
  SpellChecker::Internal::SpellCheckerCoreSettings* settings = SpellCheckerCore::instance()->settings();
  if( ( settings->onlyParseCurrentFile == true )
      && ( d->currentEditorFileName != fileName ) ) {
    return false;
  }

  if( settings->checkExternalFiles == false ) {
    return d->filesInStartupProject.contains( fileName );
  }

  return true;
}

void QmlDocumentParser::futureFinished()
{
  auto watcher = reinterpret_cast<QmlDocumentProcessor::WatcherPtr>( sender() );
  if( watcher == nullptr ) {
    return;
  }
  QString fileName;
  {
    QMutexLocker locker( &d->futureMutex );
    const auto iter = d->futureWatchers.find( watcher );
    if( iter != d->futureWatchers.end() ) {
      fileName = iter.value();
      d->futureWatchers.erase( iter );
    }
  }

  if( watcher->isCanceled() == true ) {
    watcher->deleteLater();
    return;
  }

  const WordList words = watcher->result();
  watcher->deleteLater();
  if( fileName.isEmpty() == false ) {
    emit spellcheckWordsParsed( fileName, words );
  }
}

void QmlDocumentParser::aboutToQuit()
{
  setActiveProject( nullptr );
  Core::EditorManager::instance()->disconnect( this );
  SpellCheckerCore::instance()->disconnect( this );
  this->disconnect( SpellCheckerCore::instance() );
}
