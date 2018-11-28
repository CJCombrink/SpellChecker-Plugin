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

#include "../../spellcheckercore.h"
#include "../../spellcheckercoresettings.h"
#include "../../spellcheckerconstants.h"
#include "../../Word.h"
#include "cppdocumentparser.h"
#include "cppparsersettings.h"
#include "cppparseroptionspage.h"
#include "cppparserconstants.h"
#include "cppdocumentprocessor.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditordocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/syntaxhighlighter.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/runextensions.h>

#include <QRegularExpression>
#include <QTextBlock>
#include <QApplication>
#include <QFutureWatcher>

/*! \brief Testing assert that should be used during debugging
 * but should not be made part of a release. */
#define SP_CHECK( test ) QTC_CHECK( test )
//#define SP_CHECK( test )

namespace SpellChecker {
namespace CppSpellChecker {
namespace Internal {

/*! Mime Type for the C++ doxygen files.
 * This must match the 'mime-type type' in the json.in file of this
 * plugin. */
const char MIME_TYPE_CXX_DOX[] = "text/x-c++dox";

//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

using FutureWatcherMap     = QMap<QFutureWatcher<CppDocumentProcessor::ResultType>*, QString> ;
using FutureWatcherMapIter = FutureWatcherMap::Iterator;

class CppDocumentParserPrivate {
public:
    ProjectExplorer::Project* activeProject;
    QString currentEditorFileName;
    CppParserOptionsPage* optionsPage;
    CppParserSettings* settings;
    QStringSet filesInStartupProject;
    //--
    QMutex fileQeueMutex;
    std::set<QString> filesToUpdate;
    std::set<QString> filesInProcess; /*!< Number of files that were asked to
                                * update through the CPP manager.
                                * Keeping track to try and not update too many
                                * at a time. */

    HashWords tokenHashes;
    QMutex futureMutex;
    FutureWatcherMap futureWatchers; /*!< List of future watchers created. This
                                      * list is used to cancel the futures as needed
                                      * for example when the application closes down,
                                      * the project changes or the settings changes. */

    CppDocumentParserPrivate() :
        activeProject(nullptr),
        currentEditorFileName(),
        filesInStartupProject()
    {}

    /*! \brief Get all C++ files from the \a list of files.
     *
     * This function uses the MIME Types of the passed files and check if
     * they are classified by the CppTools::ProjectFile class. If they are
     * they are regarded as C++ files.
     * If a file is unsupported the type is checked against the
     * custom MIME Type added by this plugin. */
    QStringSet getCppFiles(const QStringSet& list)
    {
        const QStringSet filteredList = Utils::filtered(list, [](const QString& file){
        const CppTools::ProjectFile::Kind kind = CppTools::ProjectFile::classify(file);
        switch(kind){
        case CppTools::ProjectFile::Unclassified:
          return false;
        case CppTools::ProjectFile::Unsupported: {
          /* Check our doxy MimeType added by this plugin */
          const Utils::MimeType mimeType = Utils::mimeTypeForFile(file);
          const QString mt = mimeType.name();
          if (mt == QLatin1String(MIME_TYPE_CXX_DOX)){
              return true;
          } else {
            return false;
          }
        }
        default:
            return true;
        }
      });

      return filteredList;
    }
    // ------------------------------------------

    static void eraseIfFound(std::set<QString>& set, const QString& file) {
      const auto setIter = set.find(file);
      if(setIter != set.end()) {
        set.erase(setIter);
      }
    }
    // ------------------------------------------
};
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

CppDocumentParser::CppDocumentParser(QObject *parent) :
    IDocumentParser(parent),
    d(new CppDocumentParserPrivate())
{
    /* Create the settings for this parser */
    d->settings = new SpellChecker::CppSpellChecker::Internal::CppParserSettings();
    d->settings->loadFromSettings(Core::ICore::settings());
    connect(d->settings, &CppParserSettings::settingsChanged, this, &CppDocumentParser::settingsChanged);
    connect(SpellCheckerCore::instance()->settings(), &SpellChecker::Internal::SpellCheckerCoreSettings::settingsChanged, this, &CppDocumentParser::settingsChanged);
    /* Crete the options page for the parser */
    d->optionsPage = new CppParserOptionsPage(d->settings, this);

    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    connect(modelManager, &CppTools::CppModelManager::documentUpdated, this, &CppDocumentParser::parseCppDocumentOnUpdate, Qt::DirectConnection);
    connect(qApp, &QApplication::aboutToQuit, this, [=](){
      /* Disconnect any signals that might still get emitted. */
      modelManager->disconnect(this);
      SpellCheckerCore::instance()->disconnect(this);
      this->disconnect(SpellCheckerCore::instance());
    }, Qt::DirectConnection);

    Core::Context context(CppEditor::Constants::CPPEDITOR_ID);
    Core::ActionContainer *cppEditorContextMenu= Core::ActionManager::createMenu(CppEditor::Constants::M_CONTEXT);
    Core::ActionContainer *contextMenu = Core::ActionManager::createMenu(Constants::CONTEXT_MENU_ID);
    cppEditorContextMenu->addSeparator(context);
    cppEditorContextMenu->addMenu(contextMenu);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &CppDocumentParser::aboutToQuit, Qt::DirectConnection);
}
//--------------------------------------------------

CppDocumentParser::~CppDocumentParser()
{
    d->settings->saveToSetting(Core::ICore::settings());
    delete d->settings;
    delete d->optionsPage;
    delete d;
}
//--------------------------------------------------

QString CppDocumentParser::displayName()
{
    return tr("C++ Document Parser");
}
//--------------------------------------------------

Core::IOptionsPage *CppDocumentParser::optionsPage()
{
    return d->optionsPage;
}
//--------------------------------------------------

void CppDocumentParser::setActiveProject(ProjectExplorer::Project *activeProject)
{
    d->activeProject = activeProject;
    d->filesInStartupProject.clear();

    /* Call reparseProject() to reset and clean up properly.
     * The logic inside will ensure that parsing is not started again
     * since there is no active project. */
    reparseProject();
}
//--------------------------------------------------

void CppDocumentParser::updateProjectFiles(QStringSet filesAdded, QStringSet filesRemoved)
{
  Q_UNUSED(filesRemoved)
  const QStringSet fileSet = d->getCppFiles(filesAdded);
  d->filesInStartupProject.unite(fileSet);
  {
    QMutexLocker locker(&d->fileQeueMutex);
    d->filesToUpdate.insert(fileSet.cbegin(), fileSet.cend());
  }
  queueFilesForUpdate();
}
//--------------------------------------------------

void CppDocumentParser::setCurrentEditor(const QString& editorFilePath)
{
    d->currentEditorFileName = editorFilePath;
}
//--------------------------------------------------

void CppDocumentParser::parseCppDocumentOnUpdate(CPlusPlus::Document::Ptr docPtr)
{
    if(docPtr.isNull() == true) {
        return;
    }

    const QString fileName = docPtr->fileName();
    const bool shouldParse = shouldParseDocument(fileName);

    bool queueMore;
    {
      QMutexLocker locker(&d->fileQeueMutex);
      /* Remove from the list to update since it will be updated now */
      d->eraseIfFound(d->filesToUpdate, fileName);
      /* Always try to queue more if there are more files to update.
       * The logic inside queueFilesForUpdate() will ensure that there
       * are no more added than what is desired. */
      queueMore = (d->filesToUpdate.size() > 0);

      /* If the file should not be parsed, remove it from the list of
       * files in process. This is needed since the queueFilesForUpdate()
       * will add it to that list when it queues it.
       * If the file should be parsed, add it to the list of files that
       * that are in process, this is used to limit the number of files
       * processed at the same time. */
      if(shouldParse == false) {
          d->eraseIfFound(d->filesInProcess, fileName);
      } else {
         d->filesInProcess.insert(fileName);
      }
    }

    if(shouldParse == true) {
      parseCppDocument(std::move(docPtr));
    }

    if(queueMore == true) {
      queueFilesForUpdate();
    }
}
//--------------------------------------------------

void CppDocumentParser::settingsChanged()
{
    /* Clear the hashes since all comments must be re parsed. */
    d->tokenHashes.clear();
    /* Re parse the project */
    reparseProject();
}
//--------------------------------------------------

void CppDocumentParser::reparseProject()
{
    /* Need to cancel all futures in process. */
    {
        QMutexLocker locker(&d->futureMutex);
        qDebug() << "FUTURES IN FLIGHT: " << d->futureWatchers.count();
        for(FutureWatcherMapIter iter = d->futureWatchers.begin(); iter != d->futureWatchers.end(); ++iter) {
          /* Can't use range for or std::for_each due to the way that a Qt QMap works... */
          iter.key()->cancel();
        }
        for(FutureWatcherMapIter iter = d->futureWatchers.begin(); iter != d->futureWatchers.end(); ++iter) {
          /* Can't use range for or std::for_each due to the way that a Qt QMap works... */
          iter.key()->waitForFinished();
        }
        d->futureWatchers.clear();
        qDebug() << "FUTURES LEFT: " << d->futureWatchers.count();
    }
    d->filesInStartupProject.clear();
    if(d->activeProject == nullptr) {
        return;
    }

    const Utils::FileNameList projectFiles = d->activeProject->files(ProjectExplorer::Project::SourceFiles);
    const QStringList fileList = Utils::transform(projectFiles, &Utils::FileName::toString);

    const QStringSet fileSet = d->getCppFiles(fileList.toSet());
    d->filesInStartupProject = fileSet;

    {
      QMutexLocker locker(&d->fileQeueMutex);
      d->filesInProcess.clear();
      d->filesToUpdate.clear();
      d->filesToUpdate = Utils::transform<std::set<QString>>(fileSet, [](const QString& string){ return string; });
    }

    queueFilesForUpdate();
}
//--------------------------------------------------

void CppDocumentParser::queueFilesForUpdate()
{
  /* Only re-parse the files that were added. */
  static CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

  QStringSet filesToUpdate;

  {
    QMutexLocker locker(&d->fileQeueMutex);
    auto fileIter = d->filesToUpdate.begin();
    while((d->filesInProcess.size() < 10)
    && (d->filesToUpdate.empty() == false)){
      const QString file = (*fileIter);
      fileIter = d->filesToUpdate.erase(fileIter);
      if(shouldParseDocument(file) == true) {
        d->filesInProcess.insert(file);
        filesToUpdate.insert(file);
      }
    }
  }

  modelManager->updateSourceFiles(filesToUpdate);
}
//--------------------------------------------------

bool CppDocumentParser::shouldParseDocument(const QString& fileName)
{
    SpellChecker::Internal::SpellCheckerCoreSettings* settings = SpellCheckerCore::instance()->settings();
    if((settings->onlyParseCurrentFile == true)
            && (d->currentEditorFileName != fileName)) {
        /* The global setting is set to only parse the current file and the
         * file asked about is not the current one, thus do not parse it. */
        return false;
    }

    if((settings->checkExternalFiles) == false) {
        /* Do not check external files so check if the file is part of the
         * active project. */
        return d->filesInStartupProject.contains(fileName);
    }

    return true;
}
//--------------------------------------------------

void CppDocumentParser::futureFinished()
{
    /* Get the watcher from the sender() of the signal that invoked this slot.
     * reinterpret_cast is used since qobject_cast is not valid of template
     * classes since the template class does not have the Q_OBJECT macro. */
    QFutureWatcher<CppDocumentProcessor::ResultType> *watcher = reinterpret_cast<QFutureWatcher<CppDocumentProcessor::ResultType>*>(sender());
    SP_CHECK(watcher != nullptr);
    if(watcher->isCanceled() == true) {
        /* Application is shutting down or settings changed etc. */
        return;
    }
    const CppDocumentProcessor::ResultType result    = watcher->result();

    QString fileName;
    {
      QMutexLocker locker(&d->futureMutex);
      /* Get the file name associated with this future and the misspelled
       * words. */
      FutureWatcherMapIter iter = d->futureWatchers.find(watcher);
      if(iter == d->futureWatchers.end()) {
          return;
      }
      fileName = iter.value();
      d->futureWatchers.erase(iter);
    }

    /* Move the new list of hashes to the member data so that
     * it can be used the next time around. Move is made explicit since
     * the LHS can be removed and the RHS will not be used again from
     * here on. */
    d->tokenHashes = std::move(result.wordHashes);

    {
      QMutexLocker locker(&d->fileQeueMutex);
      d->eraseIfFound(d->filesInProcess, fileName);
      qDebug() << "DONE: " << fileName << "(" << d->filesInProcess.size() << ", " << d->futureWatchers.count() << ", " << d->filesToUpdate.size() << ")";
    }
    queueFilesForUpdate();

    /* Now that we have all of the words from the parser, emit the signal
     * so that they will get spell checked. */
    emit spellcheckWordsParsed(fileName, result.words);
}
//--------------------------------------------------

void CppDocumentParser::aboutToQuit()
{
  setActiveProject(nullptr);
}
//--------------------------------------------------

void CppDocumentParser::parseCppDocument(CPlusPlus::Document::Ptr docPtr)
{
    const QString fileName = docPtr->fileName();
    using ResultType = CppDocumentProcessor::ResultType;
    /* Create a document parser and move it to the main thread.
     * Not sure if this is required but it seemed like a good
     * idea since this will be in a QThreadPool thread. */
    CppDocumentProcessor *parser = new CppDocumentProcessor(docPtr, d->tokenHashes, *d->settings);
    parser->moveToThread(qApp->thread());
    /* Reset the document pointer so that it can be released as soon as it is
     * done in the processor. The processor makes its own copy to keep it
     * alive. */
    docPtr.reset();

    /* Create a Future watcher that will be used to watch the future
     * promised by the processor.
     * NB: The moveToThread() call is vital. If the future is not in the
     * main thread, the finished() signals will not be delivered and the
     * processing does not work. */
    QFutureWatcher<ResultType> *watcher = new QFutureWatcher<ResultType>();
    watcher->moveToThread(qApp->thread());
    connect(watcher, &QFutureWatcher<ResultType>::finished, this, &CppDocumentParser::futureFinished, Qt::QueuedConnection);
    connect(watcher, &QFutureWatcher<ResultType>::finished, parser, &CppDocumentProcessor::deleteLater);
    /* Keep track of the watchers so that they can be cancelled as needed. */
    d->futureWatchers.insert(watcher, fileName);
    /* Add the file that will be processed to the list of files. */
    /* Run the process function in the threadpool and return the future that
     * will be used by the processor. */
    const QThread::Priority futurePriority = (fileName == d->currentEditorFileName)? QThread::HighestPriority: QThread::NormalPriority;
    QFuture<ResultType> future = Utils::runAsync(QThreadPool::globalInstance(), futurePriority, &CppDocumentProcessor::process, parser);
    watcher->setFuture(future);
}
//--------------------------------------------------

void CppDocumentParser::applySettingsToWords(const CppParserSettings& settings, const QString &string, const QStringSet &wordsInSource, WordList &words)
{
    using namespace SpellChecker::Parsers::CppParser;

    /* Filter out words that appears in the source. They are checked against the list
     * of words parsed from the file before the for loop. */
    if(settings.removeWordsThatAppearInSource == true) {
        removeWordsThatAppearInSource(wordsInSource, words);
    }

    /* Regular Expressions that might be used, defined here so that it does not get cleared in the loop.
     * They are made static const because they will be re-used a lot and will never be changed. This way
     * the construction of the objects can be done once and then be re-used. */
    static const QRegularExpression doubleRe(QStringLiteral("\\A\\d+(\\.\\d+)?\\z"));
    static const QRegularExpression hexRe(QStringLiteral("\\A0x[0-9A-Fa-f]+\\z"));
    static const QRegularExpression emailRe(QStringLiteral("\\A") + QLatin1String(SpellChecker::Parsers::CppParser::Constants::EMAIL_ADDRESS_REGEXP_PATTERN) + QStringLiteral("\\z"));
    static const QRegularExpression websiteRe(QString() + QLatin1String(SpellChecker::Parsers::CppParser::Constants::WEBSITE_ADDRESS_REGEXP_PATTERN));
    static const QRegularExpression websiteCharsRe(QString() + QLatin1String(SpellChecker::Parsers::CppParser::Constants::WEBSITE_CHARS_REGEXP_PATTERN));
    /* Word list that can be added to in the case that a word is split up into different words
     * due to some setting or rule. These words can also be checked against the settings using
     * recursion or not. It depends on the implementation that did the splitting of the
     * original word. It is done in this way so that the iterator that is currently operating
     * on the list of words does not break when new words get added during iteration */
    WordList wordsToAddInTheEnd;
    /* Iterate through the list of words using an iterator and remove words according to settings */
    WordList::Iterator iter = words.begin();
    while(iter != words.end()) {
        QString currentWord = (*iter).text;
        QString currentWordCaps = currentWord.toUpper();
        bool removeCurrentWord = false;

        /* Remove reserved words first. Although this does not depend on settings, this
         * is done here to prevent multiple iterations through the word list where possible */
        removeCurrentWord = isReservedWord(currentWord);

        if(removeCurrentWord == false) {
            /* Remove the word if it is a number, checking for floats and doubles as well. */
            if(doubleRe.match(currentWord).hasMatch() == true) {
                removeCurrentWord = true;
            }
            /* Remove the word if it is a hex number. */
            if(hexRe.match(currentWord).hasMatch() == true) {
                removeCurrentWord = true;
            }
        }

        if((removeCurrentWord == false) && (settings.checkQtKeywords == false)) {
            /* Remove the basic Qt Keywords using the isQtKeyword() function in the CppTools */
            if((CppTools::isQtKeyword(QStringRef(&currentWord)) == true)
                    || (CppTools::isQtKeyword(QStringRef(&currentWordCaps)) == true)){
                removeCurrentWord = true;
            }
            /* Remove words that Start with capital Q and the next char is also capital letter. This would
             * only apply to words longer than 2 characters long. This check is also to ensure that we do
             * not go past the size of the word */
            if(currentWord.length() > 2) {
                if((currentWord.at(0) == QLatin1Char('Q')) && (currentWord.at(1).isUpper() == true)) {
                    removeCurrentWord = true;
                }
            }

            /* Remove all caps words that start with Q_ */
            if(currentWord.startsWith(QLatin1String("Q_"), Qt::CaseSensitive) == true) {
                removeCurrentWord = true;
            }

            /* Remove qDebug() */
            if(currentWord == QLatin1String("qDebug")) {
                removeCurrentWord = true;
            }
        }

        if((settings.removeEmailAddresses == true) && (removeCurrentWord == false)) {
            if(emailRe.match(currentWord).hasMatch() == true) {
                removeCurrentWord = true;
            }
        }

        /* Attempt to remove website addresses using the websiteRe Regular Expression. */
        if((settings.removeWebsites == true) && (removeCurrentWord == false)) {
            if(websiteRe.match(currentWord).hasMatch() == true) {
                removeCurrentWord = true;
            } else if (currentWord.contains(websiteCharsRe) == true) {
                QStringList wordsSplitOnWebChars = currentWord.split(websiteCharsRe, QString::SkipEmptyParts);
                if (wordsSplitOnWebChars.isEmpty() == false) {
                    /* String is not a website, check each component now */
                    removeCurrentWord = true;
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnWebChars, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(settings, string, wordsInSource, wordsFromSplit);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                }
            }
        }

        if((settings.checkAllCapsWords == false) && (removeCurrentWord == false)) {
            /* Remove words that are all caps */
            if(currentWord == currentWordCaps) {
                removeCurrentWord = true;
            }
        }

        if((settings.wordsWithNumberOption != CppParserSettings::LeaveWordsWithNumbers) && (removeCurrentWord == false)) {
            /* Before doing anything, check if the word contains any numbers. If it does then we can go to the
             * settings to handle the word differently */
            static const QRegularExpression numberContainRe(QStringLiteral("[0-9]"));
            static const QRegularExpression numberSplitRe(QStringLiteral("[0-9]+"));
            if(currentWord.contains(numberContainRe) == true) {
                /* Handle words with numbers based on the setting that is set for them */
                if(settings.wordsWithNumberOption == CppParserSettings::RemoveWordsWithNumbers) {
                    removeCurrentWord = true;
                } else if(settings.wordsWithNumberOption == CppParserSettings::SplitWordsOnNumbers) {
                    removeCurrentWord = true;
                    QStringList wordsSplitOnNumbers = currentWord.split(numberSplitRe, QString::SkipEmptyParts);
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnNumbers, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(settings, string, wordsInSource, wordsFromSplit);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    /* Should never get here */
                    QTC_CHECK(false);
                }
            }
        }

        if((settings.wordsWithUnderscoresOption != CppParserSettings::LeaveWordsWithUnderscores) && (removeCurrentWord == false)) {
            /* Check to see if the word has underscores in it. If it does then handle according to the settings */
            if(currentWord.contains(QLatin1Char('_')) == true) {
                if(settings.wordsWithUnderscoresOption == CppParserSettings::RemoveWordsWithUnderscores) {
                    removeCurrentWord = true;
                } else if(settings.wordsWithUnderscoresOption == CppParserSettings::SplitWordsOnUnderscores) {
                    removeCurrentWord = true;
                    static const QRegularExpression underscoreSplitRe(QStringLiteral("_+"));
                    QStringList wordsSplitOnUnderScores = currentWord.split(underscoreSplitRe, QString::SkipEmptyParts);
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnUnderScores, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(settings, string, wordsInSource, wordsFromSplit);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    /* Should never get here */
                    QTC_CHECK(false);
                }
            }
        }

        /* Settings for CamelCase */
        if((settings.camelCaseWordOption != CppParserSettings::LeaveWordsInCamelCase) && (removeCurrentWord == false)) {
            /* Check to see if the word appears to be in camelCase. If it does, handle according to the settings */
            /* The check is not precise and accurate science, but a rough estimation of the word is in camelCase. This
             * will probably be updated as this gets tested. The current check checks for one or more lower case letters,
             * followed by one or more upper-case letter, followed by a lower case letter */
            static const QRegularExpression camelCaseContainsRe(QStringLiteral("[a-z]{1,}[A-Z]{1,}[a-z]{1,}"));
            static const QRegularExpression camelCaseIndexRe(QStringLiteral("[a-z][A-Z]"));
            if(currentWord.contains(camelCaseContainsRe) == true ) {
                if(settings.camelCaseWordOption == CppParserSettings::RemoveWordsInCamelCase) {
                    removeCurrentWord = true;
                } else if(settings.camelCaseWordOption == CppParserSettings::SplitWordsOnCamelCase) {
                    removeCurrentWord = true;
                    QStringList wordsSplitOnCamelCase;
                    /* Search the word for all indexes where there is a lower case letter followed by an upper case
                     * letter. This indexes are then later used to split the current word into a list of new words.
                     * 0 is added as the starting index, since the first word will start at 0. At the end the length
                     * of the word is also added, since the last word will stop at the end */
                    QList<int> indexes;
                    indexes << 0;
                    int currentIdx = 0;
                    int lastIdx = 0;
                    bool finished = false;
                    while(finished == false) {
                        currentIdx = currentWord.indexOf(camelCaseIndexRe, lastIdx);
                        if(currentIdx == -1) {
                            finished = true;
                            indexes << currentWord.length();
                        } else {
                            lastIdx = currentIdx + 1;
                            indexes << lastIdx;
                        }
                    }
                    /* Now split the word on the indexes */
                    for(int idx = 0; idx < indexes.count() - 1; ++idx) {
                        /* Get the word starting at the current index, up to the difference between the different
                         * index and the current index, since the second argument of QString::mid() is the length
                         * to extract and not the index of the last position */
                        QString word = currentWord.mid(indexes.at(idx), indexes.at(idx + 1) - indexes.at(idx));
                        wordsSplitOnCamelCase << word;
                    }
                    WordList wordsFromSplit;
                    /* Get the proper word structures for the words extracted during the split */
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnCamelCase, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(settings, string, wordsInSource, wordsFromSplit);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    /* Should never get here */
                    QTC_CHECK(false);
                }
            }
        }

        /* Words.with.dots */
        if((settings.wordsWithDotsOption != CppParserSettings::LeaveWordsWithDots) && (removeCurrentWord == false)) {
            /* Check to see if the word has dots in it. If it does then handle according to the settings */
            if(currentWord.contains(QLatin1Char('.')) == true) {
                if(settings.wordsWithDotsOption == CppParserSettings::RemoveWordsWithDots) {
                    removeCurrentWord = true;
                } else if(settings.wordsWithDotsOption == CppParserSettings::SplitWordsOnDots) {
                    removeCurrentWord = true;
                    static const QRegularExpression dotsSplitRe(QStringLiteral("\\.+"));
                    QStringList wordsSplitOnDots = currentWord.split(dotsSplitRe, QString::SkipEmptyParts);
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnDots, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(settings, string, wordsInSource, wordsFromSplit);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    /* Should never get here */
                    QTC_CHECK(false);
                }
            }
        }

        /* Remove the current word if it should be removed. The word will get removed in place. The
         * erase() function on the list will return an iterator to the next element. In this case,
         * the iterator should not be incremented and the while loop should continue to the next element. */
        if(removeCurrentWord == true) {
            iter = words.erase(iter);
        } else {
            ++iter;
        }
    }
    /* Add the words that should be added in the end to the list of words */
    words.append(wordsToAddInTheEnd);
}
//--------------------------------------------------

//--------------------------------------------------

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker
