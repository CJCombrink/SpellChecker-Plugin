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
#include "../../spellcheckerconstants.h"
#include "../../Word.h"
#include "cppdocumentparser.h"
#include "cppparsersettings.h"
#include "cppparseroptionspage.h"
#include "cppparserconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cppdoxygen.h>
#include <cplusplus/Overview.h>
#include <cppeditor/cppeditorconstants.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/syntaxhighlighter.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <QRegularExpression>
#include <QTextBlock>

namespace SpellChecker {
namespace CppSpellChecker {
namespace Internal {

class CppDocumentParserPrivate {
public:
    ProjectExplorer::Project* startupProject;
    Core::IEditor* currentEditor;
    QString currentEditorFileName;
    CppParserOptionsPage* optionsPage;
    CppParserSettings* settings;
    QStringList filesInStartupProject;
    QStringList sourceFilesInStartupProject;

    CppDocumentParserPrivate() :
        startupProject(NULL),
        currentEditor(NULL),
        currentEditorFileName(),
        filesInStartupProject(),
        sourceFilesInStartupProject()
    {}
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
    connect(d->settings, SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
    /* Crete the options page for the parser */
    d->optionsPage = new CppParserOptionsPage(d->settings, this);

    CppTools::CppModelManagerInterface *modelManager = CppTools::CppModelManagerInterface::instance();
    connect(modelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)), this, SLOT(parseCppDocumentOnUpdate(CPlusPlus::Document::Ptr)), Qt::DirectConnection);

    Core::Context context(CppEditor::Constants::C_CPPEDITOR);
    Core::ActionContainer *cppEditorContextMenu= Core::ActionManager::createMenu(CppEditor::Constants::M_CONTEXT);
    Core::ActionContainer *contextMenu = Core::ActionManager::createMenu(Constants::CONTEXT_MENU_ID);
    cppEditorContextMenu->addSeparator(context);
    cppEditorContextMenu->addMenu(contextMenu);
}
//--------------------------------------------------

CppDocumentParser::~CppDocumentParser()
{
    d->settings->saveToSetting(Core::ICore::settings());
    delete d->settings;
    delete d;
}
//--------------------------------------------------

FileWordList CppDocumentParser::parseFiles(const QStringList &fileNames)
{
    qDebug() << "in CppDocumentParser::parseFiles: " << fileNames << "(" << fileNames.count() << ")";
    FileWordList fileWords;
    WordList words;
    return fileWords;

    /* Get the Snapshot from the Model Manager Interface instance. This snapshot is then used
     * to get a C++ Doc pointer from for the given file name. If the file is a C++ file then
     * the snapshot will return a valid pointer for the given file. Otherwise the file is not a
     * valid C++ document and it will be ignored by this parser */
    const CPlusPlus::Snapshot &snapshot = CppTools::CppModelManagerInterface::instance()->snapshot();

    CPlusPlus::Snapshot::const_iterator itr;
    for(itr = snapshot.begin(); itr != snapshot.end(); ++itr) {
        qDebug() << "File: " << itr.key() << " (" << itr.value()->fileName() << ")";
        qDebug() << "size = " << CppTools::CppModelManagerInterface::instance()->snapshot().size();
    }

    /* Iterate all of the files in the list of files and get their C++ document pointers. Parse
     * the valid C++ documents */
    foreach(const QString& fileName, fileNames) {
        qApp->processEvents();
        qDebug() << "size = " << CppTools::CppModelManagerInterface::instance()->snapshot().size();
        CPlusPlus::Document::Ptr doc = snapshot.document(fileName);
        qDebug() << "Snapshot size: " << snapshot.size();
        qDebug() << "doc == NULL: " << (doc == NULL);
        qDebug() << "Snapshot contains: " << snapshot.contains(fileName);
        if(doc == NULL) {
            /* The current file is not a valid C++ file. Continue to the next file */
            continue;
        }
        words = parseAndSpellCheckCppDocument(doc);
        if(words.isEmpty() == true) {
            /* No spelling mistakes found in the current file, continue to the next file */
            continue;
        }
        /* For the given file, there were spelling mistakes. Insert the file along with the
         * words into the list that will be returned by this function */
        fileWords.insert(fileName, words);
    }
    return fileWords;
}
//--------------------------------------------------

WordList CppDocumentParser::parseFile(const QString &fileName)
{
    qDebug() << "CppDocParser::parseFile(): fileName" << fileName;
    /* See comments in parseFiles() */
    const CPlusPlus::Snapshot &snapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
    CPlusPlus::Document::Ptr doc = snapshot.document(fileName);
    qDebug() << "doc == NULL: " << (doc == NULL);
    if(doc == NULL) {
        return WordList();
    }
    return parseAndSpellCheckCppDocument(doc);
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

void CppDocumentParser::setStartupProject(ProjectExplorer::Project *startupProject)
{
    d->startupProject = startupProject;
    d->filesInStartupProject.clear();
    d->sourceFilesInStartupProject.clear();
    if(d->startupProject == NULL) {
        return;
    }
    /* Get all the files in the startup project */
    CppTools::CppModelManagerInterface *modelManager = CppTools::CppModelManagerInterface::instance();
    Q_ASSERT(modelManager != NULL); // NOT SURE IF THIS IS POSSIBLE
    CppTools::CppModelManagerInterface::ProjectInfo startupProjectInfo = modelManager->projectInfo(d->startupProject);
    d->filesInStartupProject = startupProjectInfo.project().data()->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
    d->sourceFilesInStartupProject = startupProjectInfo.sourceFiles();
}
//--------------------------------------------------

void CppDocumentParser::setCurrentEditor(Core::IEditor *editor)
{
    d->currentEditorFileName.clear();
    d->currentEditor = editor;
    if(d->currentEditor == NULL) {
        return;
    }
    d->currentEditorFileName = editor->document()->filePath();

    CppTools::CppModelManagerInterface *modelManager = CppTools::CppModelManagerInterface::instance();
    if(modelManager->isCppEditor(editor) == false) {
        return;
    }
    modelManager->updateSourceFiles(QStringList() << d->currentEditorFileName);
#ifdef true
    /* I have tried to get a CPlusPlus::Document::Ptr from the editor but for some
     * reason when trying this Qt Creator crashes when calling
     * docPtr->translationUnit()->commentCount() on the ptr. For now asking
     * the CppModelManagerInterface to update the file so that it can be parsed. */
    CPlusPlus::Snapshot snapshot = modelManager->snapshot();
    CPlusPlus::Document::Ptr docPtr = snapshot.document(d->currentEditorFileName);
    if(docPtr.isNull() == true) {
        return;
    }
    parseCppDocumentOnUpdate(docPtr);
#endif /* true */
}
//--------------------------------------------------

void CppDocumentParser::parseCppDocumentOnUpdate(CPlusPlus::Document::Ptr docPtr)
{
    if(docPtr == NULL) {
        return;
    }

    QString fileName = docPtr->fileName();
    if(d->currentEditorFileName != fileName) {
        return;
    }

    if(shouldParseDocument(fileName) == false) {
        return;
    }
    WordList words = parseAndSpellCheckCppDocument(docPtr);
    SpellCheckerCore::instance()->addWordsWithSpellingMistakes(fileName, words);

    /* Underlining the mistakes does not work as intended. Applying the format to the
     * word does work but it gets cleared immediately (the underline flashes in and
     * out). It does stay if changing editors and changing back. Since this is not
     * fully working this is for now removed using a preprocessor macro. */
#define ATTEMPT_UNDERLINE_MISTAKES
#ifdef ATTEMPT_UNDERLINE_MISTAKES
    /* Apply the formatting for spelling mistakes */
    if(d->currentEditor == NULL) {
        return;
    }
    TextEditor::BaseTextEditor* baseText = qobject_cast<TextEditor::BaseTextEditor*>(d->currentEditor);
    if(baseText == NULL) {
        return;
    }
    TextEditor::SyntaxHighlighter *highlighter = baseText->baseTextDocument()->syntaxHighlighter();
        // Clear all additional formats since they may have changed
    QTextBlock b = baseText->baseTextDocument()->document()->firstBlock();
    WordList::ConstIterator wordIter;
    unsigned int line = 0;
    while (b.isValid()) {
        line++;
        wordIter = words.constBegin();
        QList<QTextLayout::FormatRange> mistakeFormats;
        while(wordIter != words.constEnd()) {
            const Word& currentWord = wordIter.value();
            if(currentWord.lineNumber == line) {
                /* Set the formatting for the word in the block */
                QTextLayout::FormatRange mistake;
                mistake.start = currentWord.columnNumber - 1;
                mistake.length = currentWord.length;
                mistake.format.setFontUnderline(true);
                mistake.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
                mistake.format.setUnderlineColor(Qt::red);
                mistakeFormats.append(mistake);
            }
            ++wordIter;
        }

        if(mistakeFormats.count() != 0) {
            highlighter->setExtraAdditionalFormats(b, mistakeFormats);
        }
        b = b.next();
    }
#endif /* ATTEMPT_UNDERLINE_MISTAKES */
}
//--------------------------------------------------

void CppDocumentParser::settingsChanged()
{
    /* Depending on the global parse setting, parse current file or project */
    setCurrentEditor(d->currentEditor);
}
//--------------------------------------------------

void CppDocumentParser::reparseProject()
{
    if(d->startupProject == NULL) {
        return;
    }

    CppTools::CppModelManagerInterface *modelManager = CppTools::CppModelManagerInterface::instance();
    Q_ASSERT(modelManager != NULL); // NOT SURE IF THIS IS POSSIBLE
    CppTools::CppModelManagerInterface::ProjectInfo startupProjectInfo = modelManager->projectInfo(d->startupProject);
    QStringList filesInProject = startupProjectInfo.project().data()->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
    modelManager->updateSourceFiles(filesInProject);
}
//--------------------------------------------------

bool CppDocumentParser::shouldParseDocument(const QString& fileName)
{
    return (d->sourceFilesInStartupProject.contains(fileName)
            && (d->filesInStartupProject.contains(fileName)));
}
//--------------------------------------------------

WordList CppDocumentParser::parseAndSpellCheckCppDocument(CPlusPlus::Document::Ptr docPtr)
{
    QTime time;
    time.start();
    if(docPtr.isNull() == true) {
        return WordList();
    }

    WordList finalWords;
    QStringList wordsInSource;
    /* If the setting is set to remove words from the list based on words found in the source,
     * parse the source file and then remove all words found in the source files from the list
     * of words that will be checked. */
    if(d->settings->removeWordsThatAppearInSource == true) {
        /* First get all words that does appear in the current source file. These words only
         * include variables and their types */
        getWordsThatAppearInSource(docPtr, wordsInSource);
    }

    for(unsigned int comment = 0; comment < docPtr->translationUnit()->commentCount(); ++comment) {
        WordList words;
        CPlusPlus::Token token = docPtr->translationUnit()->commentAt(comment);
        bool isDoxygenComment = ((token.kind() == CPlusPlus::T_DOXY_COMMENT)
                                 || (token.kind() == CPlusPlus::T_CPP_DOXY_COMMENT));
        QString commentString = QString::fromUtf8(docPtr->utf8Source().mid(token.bytesBegin(), token.bytes()).trimmed());
        /* Tokenize the comment to extract words that should be checked from the comment */
        tokenizeWords(docPtr->fileName(), commentString, token.bytesBegin(), docPtr->translationUnit(), words);
        /* Filter out words based on settings */
        applySettingsToWords(commentString, words, isDoxygenComment);
        /* If there are no words to check at this stage it probably means all potential words were removed
         * due to settings. If this is the case we can continue to the next comment without doing anything
         * else for the current comment. */
        if(words.count() == 0) {
            continue;
        }
        /* Filter out words that appears in the source. They are checked against the list
         * of words parsed from the file before the for loop. */
        if(d->settings->removeWordsThatAppearInSource == true) {
            removeWordsThatAppearInSource(wordsInSource, words);
        }

        /* The list that we are left with, is the list of words that should be checked for potential
         * spelling mistakes. Now check the words for spelling mistakes. */
         SpellCheckerCore::instance()->spellCheckWords(commentString, words);
         finalWords.append(words);
    }
    return finalWords;
}
//--------------------------------------------------

void CppDocumentParser::tokenizeWords(const QString& fileName, const QString &comment, unsigned int commentStart, const CPlusPlus::TranslationUnit * const translationUnit, WordList &words)
{
    int strLength = comment.length();
    bool busyWithWord = false;
    int wordStartPos = 0;
    bool endOfWord = false;

    /* Iterate through all of the characters in the comment and extract words from them.
     * Words are split up by non-word characters and is checked using the isEndOfCurrentWord()
     * function. The end condition is deliberately set to continue past the length of the
     * comment so that a word in progress is stopped and extracted when the comment ends */
    for(int currentPos = 0; currentPos <= strLength; ++currentPos) {
        /* Check if the current character is the end of a word character. */
        endOfWord = isEndOfCurrentWord(comment, currentPos);
        if((endOfWord == false) && (busyWithWord == false)){
            wordStartPos = currentPos;
            busyWithWord = true;
        }

        if((busyWithWord == true) && (endOfWord == true)) {
            Word word;
            word.fileName = fileName;
            word.text = comment.mid(wordStartPos, currentPos - wordStartPos);
            word.start = wordStartPos;
            word.end = currentPos;
            word.length = currentPos - wordStartPos;
            translationUnit->getPosition(commentStart + wordStartPos, &word.lineNumber, &word.columnNumber);
            words.append(word);
            busyWithWord = false;
            wordStartPos = 0;
        }

    }
    return;
}
//--------------------------------------------------

void CppDocumentParser::applySettingsToWords(const QString &comment, WordList &words, bool isDoxygenComment)
{
    /* Regular Expressions that might be used, defined here so that it does not get cleared in the loop */
    QRegularExpression doubleRe(QLatin1String("\\A\\d+(\\.\\d+)?\\z"));
    QRegularExpression hexRe(QLatin1String("\\A0x[0-9A-Fa-f]+\\z"));
    QRegularExpression emailRe(QLatin1String("\\A") + QLatin1String(SpellChecker::Parsers::CppParser::Constants::EMAIL_ADDRESS_REGEXP_PATTERN) + QLatin1String("\\z"));
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

        if((removeCurrentWord == false) && (d->settings->checkQtKeywords == false)) {
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

        if((d->settings->removeEmailAddresses == true) && (removeCurrentWord == false)) {
            if(emailRe.match(currentWord).hasMatch() == true) {
                removeCurrentWord = true;
            }
        }

        if((d->settings->checkAllCapsWords == false) && (removeCurrentWord == false)) {
            /* Remove words that are all caps */
            if(currentWord == currentWordCaps) {
                removeCurrentWord = true;
            }
        }

        if((d->settings->wordsWithNumberOption != CppParserSettings::LeaveWordsWithNumbers) && (removeCurrentWord == false)) {
            /* Before doing anything, check if the word contains any numbers. If it does then we can go to the
             * settings to handle the word differently */
            if(currentWord.contains(QRegExp(QLatin1String("[0-9]"))) == true) {
                /* Handle words with numbers based on the setting that is set for them */
                if(d->settings->wordsWithNumberOption == CppParserSettings::RemoveWordsWithNumbers) {
                    removeCurrentWord = true;
                } else if(d->settings->wordsWithNumberOption == CppParserSettings::SplitWordsOnNumbers) {
                    removeCurrentWord = true;
                    QStringList wordsSplitOnNumbers = currentWord.split(QRegExp(QLatin1String("[0-9]+")), QString::SkipEmptyParts);
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnNumbers, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(comment, wordsFromSplit, isDoxygenComment);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    Q_ASSERT(false);
                }
            }
        }

        if((d->settings->wordsWithUnderscoresOption != CppParserSettings::LeaveWordsWithUnderscores) && (removeCurrentWord == false)) {
            /* Check to see if the word has underscores in it. If it does then handle according to the settings */
            if(currentWord.contains(QLatin1Char('_')) == true) {
                if(d->settings->wordsWithUnderscoresOption == CppParserSettings::RemoveWordsWithUnderscores) {
                    removeCurrentWord = true;
                } else if(d->settings->wordsWithUnderscoresOption == CppParserSettings::SplitWordsOnUnderscores) {
                    removeCurrentWord = true;
                    QStringList wordsSplitOnUnderScores = currentWord.split(QRegExp(QLatin1String("_+")), QString::SkipEmptyParts);
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnUnderScores, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(comment, wordsFromSplit, isDoxygenComment);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    Q_ASSERT(false);
                }
            }
        }

        /* ADD THE SETTINGS FOR CamelCase */
        if((d->settings->camelCaseWordOption != CppParserSettings::LeaveWordsInCamelCase) && (removeCurrentWord == false)) {
            /* Check to see if the word appears to be in camelCase. If it does, handle according to the settings */
            /* The check is not precise and accurate science, but a rough estimation of the word is in camelCase. This
             * will probably be updated as this gets tested. The current check checks for one or more lower case letters,
             * followed by one or more uppercase letter, followed by a lower case letter */
            if(currentWord.contains(QRegExp(QLatin1String("[a-z]{1,}[A-Z]{1,}[a-z]{1,}"))) == true ) {
                if(d->settings->camelCaseWordOption == CppParserSettings::RemoveWordsInCamelCase) {
                    removeCurrentWord = true;
                } else if(d->settings->camelCaseWordOption == CppParserSettings::SplitWordsOnCamelCase) {
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
                        currentIdx = currentWord.indexOf(QRegExp(QLatin1String("[a-z][A-Z]")), lastIdx);
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
                    applySettingsToWords(comment, wordsFromSplit, isDoxygenComment);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    Q_ASSERT(false);
                }
            }
        }

        /* Words.with.dots */
        if((d->settings->wordsWithDotsOption != CppParserSettings::LeaveWordsWithDots) && (removeCurrentWord == false)) {
            /* Check to see if the word has dots in it. If it does then handle according to the settings */
            if(currentWord.contains(QLatin1Char('.')) == true) {
                if(d->settings->wordsWithDotsOption == CppParserSettings::RemoveWordsWithDots) {
                    removeCurrentWord = true;
                } else if(d->settings->wordsWithDotsOption == CppParserSettings::SplitWordsOnDots) {
                    removeCurrentWord = true;
                    QStringList wordsSplitOnDots = currentWord.split(QRegExp(QLatin1String("\\.+")), QString::SkipEmptyParts);
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnDots, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(comment, wordsFromSplit, isDoxygenComment);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    Q_ASSERT(false);
                }
            }
        }

        /* Doxygen comments */
        if((isDoxygenComment == true) && (removeCurrentWord == false)) {
            if(comment.at((*iter).start - 1) == QLatin1Char('\\')) {
                int doxyClass = CppTools::classifyDoxygenTag(currentWord.unicode(), currentWord.size());
                if(doxyClass != CppTools::T_DOXY_IDENTIFIER) {
                    removeCurrentWord = true;
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

void CppDocumentParser::getWordsThatAppearInSource(CPlusPlus::Document::Ptr docPtr, QStringList &wordsInSource)
{
    if(docPtr == NULL) {
        Q_ASSERT(docPtr != NULL);
        return;
    }

    unsigned total = docPtr->globalSymbolCount();
    CPlusPlus::Overview overview;
    for (unsigned i = 0; i < total; ++i) {
        CPlusPlus::Symbol *symbol = docPtr->globalSymbolAt(i);
        getListOfWordsFromSourceRecursive(wordsInSource, symbol, overview);
    }
    /* Remove all duplicates from the list of words that occur in the source. Also sort the list.
     * Not sure which is better, to first remove and then sort, or to first sort and then remove */
    wordsInSource.removeDuplicates();
    wordsInSource.sort();
}
//--------------------------------------------------

void CppDocumentParser::getListOfWordsFromSourceRecursive(QStringList &words, const CPlusPlus::Symbol *symbol, const CPlusPlus::Overview &overview)
{
    /* Get the pretty name and type for the current symbol. This name is then split up into
     * different words that are added to the list of words that appear in the source */
    QString name = overview.prettyName(symbol->name()).trimmed();
    QString type = overview.prettyType(symbol->type()).trimmed();
    words << getPossibleNamesFromString(name);
    words << getPossibleNamesFromString(type);

    /* Go to the next level into the scope of the symbol and get the words from that level as well*/
    const CPlusPlus::Scope *scope = symbol->asScope();
    if (scope != NULL) {
        CPlusPlus::Scope::iterator cur = scope->firstMember();
        while (cur != scope->lastMember()) {
            const CPlusPlus::Symbol *curSymbol = *cur;
            ++cur;
            if (!curSymbol) {
                continue;
            }
            getListOfWordsFromSourceRecursive(words, curSymbol, overview);
        }
    }
}
//--------------------------------------------------

QStringList CppDocumentParser::getPossibleNamesFromString(const QString& string)
{
    QStringList possibleNames;
    QRegExp nameRegExp(QLatin1String("(\\w+)"));
    int pos = 0;
    while ((pos = nameRegExp.indexIn(string, pos)) != -1) {
        possibleNames << nameRegExp.cap(1);
        pos += nameRegExp.matchedLength();
    }
    return possibleNames;
}
//--------------------------------------------------

void CppDocumentParser::removeWordsThatAppearInSource(const QStringList &wordsInSource, WordList &words)
{
    /* Hopefully all words that are the same would be together in the WordList because of
     * the nature of the QMultiHash. This would make removing multiple instances of the same
     * word faster.
     *
     * The idea is to save the last word that was removed, and then for the next word,
     * first compare it to the last word removed, and if it is the same, remove it from the list. Else
     * search for the word in the wordsInSource list. The reasoning is that the first compare will be
     * faster than the search in the wordsInSource list. But with this the overhead is added that now
     * there is perhaps one more compare for each word that is not present in the source. Perhaps this
     * can then be slower because the probability is that there will be more words not in the source than
     * there are duplicate words that are in the source. For now this will be left this way but more
     * benchmarking needs to be done to optimize.
     *
     * An initial test using QTime::start() and QTime::elapsed() showed the same speed of 3ms for either
     * option for about 1425 potential words checked in 98 words occurring in the source. 148 Words were
     * removed for this test case. NOTE: Due to the inaccuracy of the timer on windows a better test
     * will be performed in future. */
#ifdef USE_MULTI_HASH
    WordList::Iterator iter = words.begin();
    QString lastWordRemoved(QLatin1String(""));
    while(iter != words.end()) {
        if(iter.key() == lastWordRemoved) {
            iter = words.erase(iter);
        } else if(wordsInSource.contains(iter.key())) {
            lastWordRemoved = iter.key();
            /* The word does appear in the source, thus remove it from the list of
             * potential words that must be checked */
            iter = words.erase(iter);
        } else {
            ++iter;
        }
    }
#else /* USE_MULTI_HASH */
    WordList::Iterator iter = words.begin();
    while(iter != words.end()) {
        if(wordsInSource.contains((*iter).text)) {
            /* The word does appear in the source, thus remove it from the list of
             * potential words that must be checked */
            iter = words.erase(iter);
        } else {
            ++iter;
        }
    }
#endif /* USE_MULTI_HASH */
}
//--------------------------------------------------

bool CppDocumentParser::isEndOfCurrentWord(const QString &comment, int currentPos)
{
    /* Check to see if the current position is past the length of the comment. If this
     * is the case, then clearly it is the end of the current word */
    if(currentPos >= comment.length()) {
        return true;
    }

    const QChar& currentChar = comment[currentPos];

    /* Use a regular expression to check if the current character is a word character or
     * or an apostrophe. Some settings might change what the end of a word actually is.
     * For some settings an underscore will be considered as the end of a word */
    QRegExp wordChars(QLatin1String("[a-zA-Z0-9_]"));
    if(wordChars.indexIn(currentChar) != -1){
        /* The character is in the list given, thus it is not the end of a word */
        return false;
    }

    /* Check for an apostrophe in a word. This is for words like we're. Not all
     * apostrophes are part of a word, like words that starts with and end with */
    if(currentChar == QLatin1Char('\'')) {
        /* Do some range checking, is this is the first or last character then
         * this is not part of a word, thus it is the end of the current word */
        if((currentPos == 0) || (currentPos == (comment.length() - 1))) {
            return true;
        }
        if((comment.at(currentPos + 1).isLetter() == true)
                && (comment.at(currentPos - 1).isLetter() == true)) {
            return false;
        }
    }

    /* For words with '.' in, such as abbreviations and email addresses */
    if(currentChar == QLatin1Char('.')) {
        if((currentPos == 0) || (currentPos == (comment.length() - 1))) {
            return true;
        }
        QRegExp wordChars(QLatin1String("\\w"));
        if((wordChars.indexIn(comment.at(currentPos + 1)) != -1)
                &&(wordChars.indexIn(comment.at(currentPos - 1)) != -1)) {
            return false;
        }
    }

    /* For word with @ in: Email address */
    if(currentChar == QLatin1Char('@')) {
        if((currentPos == 0) || (currentPos == (comment.length() - 1))) {
            return true;
        }
        QRegExp wordChars(QLatin1String("\\w"));
        if((wordChars.indexIn(comment.at(currentPos + 1)) != -1)
                &&(wordChars.indexIn(comment.at(currentPos - 1)) != -1)) {
            return false;
        }
    }

    return true;
}
//--------------------------------------------------

bool CppDocumentParser::isReservedWord(const QString &word)
{
    /* Trying to optimize the check using the same method as used
     * in the cpptoolsreuse.cpp file in the CppTools plugin. */
    switch (word.length()) {
    case 3:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'C':
            if(word.toUpper() == QLatin1String("CPP"))
                return true;
            break;
        }
    case 4:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'E':
            if(word.toUpper() == QLatin1String("ENUM"))
                return true;
            break;
        }
    case 6:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'S':
            if(word.toUpper() == QLatin1String("STRUCT"))
                return true;
            break;
        }
        break;
    case 7:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'D':
            if(word.toUpper() == QLatin1String("DOXYGEN"))
                return true;
            break;
        }
        break;
    case 9:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'N':
            if(word.toUpper() == QLatin1String("NAMESPACE"))
                return true;
            break;
        }
        break;
    default:
        break;
    }
    return false;
}
//--------------------------------------------------

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker
