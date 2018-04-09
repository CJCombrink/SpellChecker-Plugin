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

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cppdoxygen.h>
#include <cplusplus/Overview.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditordocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/syntaxhighlighter.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <utils/algorithm.h>

#include <QRegularExpression>
#include <QTextBlock>

namespace SpellChecker {
namespace CppSpellChecker {
namespace Internal {

/*! \brief Class containing the words of a specific token.
 *
 * This class is used to store the words for a specific token as well as the
 * start position (line and column) of the token. The line and column is used
 * for keeping the offset correct if a token moved due to new tokens or text.
 * This is then used to adjust the line and column numbers of the words to the
 * correct locations. */
class TokenWords {
public:
    quint32 line;
    quint32 col;
    WordList words;

    TokenWords(quint32 l = 0, quint32 c = 0, const WordList &w = WordList()) :
        line(l),
        col(c),
        words(w) {}
};
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

class CppDocumentParserPrivate {
public:
    ProjectExplorer::Project* activeProject;
    QString currentEditorFileName;
    CppParserOptionsPage* optionsPage;
    CppParserSettings* settings;
    QStringList filesInStartupProject;
    const QRegularExpression cppRegExp;
    HashWords tokenHashes;

    CppDocumentParserPrivate() :
        activeProject(nullptr),
        currentEditorFileName(),
        filesInStartupProject(),
        cppRegExp(QLatin1String(SpellChecker::Parsers::CppParser::Constants::CPP_SOURCE_FILES_REGEXP_PATTERN), QRegularExpression::CaseInsensitiveOption)
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
    connect(d->settings, &CppParserSettings::settingsChanged, this, &CppDocumentParser::settingsChanged);
    connect(SpellCheckerCore::instance()->settings(), &SpellChecker::Internal::SpellCheckerCoreSettings::settingsChanged, this, &CppDocumentParser::settingsChanged);
    /* Crete the options page for the parser */
    d->optionsPage = new CppParserOptionsPage(d->settings, this);

    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    connect(modelManager, &CppTools::CppModelManager::documentUpdated, this, &CppDocumentParser::parseCppDocumentOnUpdate, Qt::DirectConnection);

    Core::Context context(CppEditor::Constants::CPPEDITOR_ID);
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
    if(d->activeProject == nullptr) {
        return;
    }
    reparseProject();
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
    QString fileName = docPtr->fileName();
    if(shouldParseDocument(fileName) == false) {
        return;
    }

    WordList words = parseCppDocument(docPtr);
    /* Now that we have all of the words from the parser, emit the signal
     * so that they will get spell checked. */
    emit spellcheckWordsParsed(fileName, words);
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
    d->filesInStartupProject.clear();
    if(d->activeProject == nullptr) {
        return;
    }
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QStringList list = Utils::transform(
                d->activeProject->files(ProjectExplorer::Project::SourceFiles), &Utils::FileName::toString);
    d->filesInStartupProject = list.filter(d->cppRegExp);
    modelManager->updateSourceFiles(d->filesInStartupProject.toSet());
}
//--------------------------------------------------

bool CppDocumentParser::shouldParseDocument(const QString& fileName)
{
    if((SpellCheckerCore::instance()->settings()->onlyParseCurrentFile == true)
            && (d->currentEditorFileName != fileName)) {
        /* The global setting is set to only parse the current file and the
         * file asked about is not the current one, thus do not parse it. */
        return false;
    }

    if((SpellCheckerCore::instance()->settings()->checkExternalFiles) == false) {
        /* Do not check external files so check if the file is part of the
         * active project. */
        return d->filesInStartupProject.contains(fileName);
    }

    /* Any file can go, project files or external files so just make sure that the
     * current one is at least a C++ source file. */
    return fileName.contains(d->cppRegExp);
}
//--------------------------------------------------

WordList CppDocumentParser::parseCppDocument(CPlusPlus::Document::Ptr docPtr)
{
    if(docPtr.isNull() == true) {
        return WordList();
    }
    /* Get the translation unit for the document. From this we can
     * iterate the tokens to get the string literals as well as the
     * comments. */
    CPlusPlus::TranslationUnit *trUnit = docPtr->translationUnit();
    if(trUnit == nullptr) {
        return WordList();
    }

    /* Make a local copy of the last list of hashes. A local copy is made and used
     * as the input the tokenize function, but a new list is returned from the
     * tokenize function. If this is not done the list of hashes can grow forever
     * and cause a huge increase in memory. Doing it this way ensure that the
     * list only contains hashes of tokens that are present in during the last run
     * and will not contain old and invalid hashes. It will cause the parsing of a
     * different file than the previous run to be less efficient but if a file is
     * parsed multiple times, one after the other, it will result in a large speed up
     * and this will mostly be the case when editing a file. For this reason the initial
     * project parse on start up can be slower. */
    const HashWords tokenHashesIn = d->tokenHashes;
    /* Empty one that will become the new one. */
    HashWords tokenHashesOut;

    WordList parsedWords;
    QSet<QString> wordsInSource;
    /* If the setting is set to remove words from the list based on words found in the source,
     * parse the source file and then remove all words found in the source files from the list
     * of words that will be checked. */
    if(d->settings->removeWordsThatAppearInSource == true) {
        /* First get all words that does appear in the current source file. These words only
         * include variables and their types */
        getWordsThatAppearInSource(docPtr, wordsInSource);
    }

    if(d->settings->whatToCheck.testFlag(CppParserSettings::CheckStringLiterals) == true) {
        /* Parse string literals */
        unsigned int tokenCount = trUnit->tokenCount();
        for(unsigned int idx = 0; idx < tokenCount; ++idx) {
            const CPlusPlus::Token& token = trUnit->tokenAt(idx);
            if(token.isStringLiteral() == true) {
                if(token.expanded() == true) {
                    /* Expanded literals comes from macros. These are not checked since they can be the
                     * result of a macro like '__LINE__'. A user is not interested in such literals.
                     * The input arguments to Macros are more usable like MY_MAC("Some String").
                     * The macro arguments are checked after the normal literals are checked. */
                    continue;
                }

                /* The String Literal is not expanded thus handle it like a comment is handled. */
                parseToken(docPtr, token, trUnit, wordsInSource, /* Comment */ false, /* Doxygen */ false, parsedWords, tokenHashesIn, tokenHashesOut);
            }
        }
        /* Parse macros */
        /* Get the macros from the document pointer. The arguments of the macro will then be parsed
         * and checked for spelling mistakes.
         * Since the TranslationUnit::getPosition() is not usable with the Macro Uses a manual method to extract
         * the literals and their line and column positions are implemented. This involves getting the unprocessed
         * source from the document for the macro and tracking the byte offsets manually from there.
         * An alternative implementation can involve using the Snapshot::preprocessedDocument(), which makes use of the FastPreprocessor
         * and the AST visitors.
         * For more information around a discussion on the mailing list around this issue refer to the following
         * link: http://comments.gmane.org/gmane.comp.lib.qt.creator/11853 */
        QList<CPlusPlus::Document::MacroUse> macroUse = docPtr->macroUses();
        if(macroUse.count() != 0) {
            CppTools::CppModelManager *cppModelManager = CppTools::CppModelManager::instance();
            CppTools::CppEditorDocumentHandle *cppEditorDocument = cppModelManager->cppEditorDocument(docPtr->fileName());
            if(cppEditorDocument != nullptr) {
                const QByteArray source = cppEditorDocument->contents();

                for(const CPlusPlus::Document::MacroUse& mac: macroUse) {
                    const QVector<CPlusPlus::Document::Block>& args = mac.arguments();
                    if((mac.isFunctionLike() == false)
                            || (args.count() == 0)) {
                        /* The argument is not function like or there are no arguments */
                        continue;
                    }
                    /* The line number of the macro is used as the reference. */
                    int line  = mac.beginLine();
                    /* Get the start of the line from the source. From this start the offset to the
                     * words will be calculated. */
                    int start = source.lastIndexOf("\n", mac.bytesBegin());
                    /* Get the end index of the last argument of the macro. */
                    int end   = args.last().bytesEnd();
                    if(end == 0) {
                        /* This will happen on an empty argument, for example Q_ASSERT() */
                        continue;
                    }

                    /* Get the full macro from the start of the line until the last byte of the last
                     * argument. */
                    QByteArray macroBytes = source.mid(start, end - start);
                    /* Quick check to see if there are any string literals in the macro text.
                     * if the are this check can be a waist, but if not this can speed up the check by
                     * avoiding an unneeded regular expression. */
                    if(macroBytes.contains('\"') == false) {
                        continue;
                    }
                    /* Get the byte offsets inside the macro bytes for each line break inside the macro.
                     * This will be used during the extraction to get the correct lines relative to the
                     * line that the macro started on. */
                    QList<int> lineIndexes;
                    /* The search above for the start include the new line character so the start for other
                     * new lines must start from index 1 so that it is not also included. */
                    int startSearch = 1;
                    while(true) {
                        int idx = macroBytes.indexOf('\n', startSearch);
                        if(idx < 0) {
                            /* No more new lines, break out */
                            break;
                        }
                        lineIndexes << idx;
                        /* Must increment the start of the next search otherwise it will be found on
                       * that index. */
                        startSearch = idx + 1;
                    }
                    /* The end is also added to simplify some of the checks further on. No character can
                     * fall outside of the end. */
                    lineIndexes << end;
                    int lineBreak = lineIndexes.front();
                    lineIndexes.pop_front();
                    int colOffset = 0;

                    /* Use a regular expression to get all string literals from the macro and its arguments. */
                    static const QRegularExpression regExp(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""));
                    QRegularExpressionMatchIterator regExpIter = regExp.globalMatch(QString::fromLatin1(macroBytes));
                    while(regExpIter.hasNext() == true) {
                        QRegularExpressionMatch match = regExpIter.next();
                        QString tokenString = match.captured(0);
                        int capStart = match.capturedStart(0);
                        /* Check if the literal starts on the next line from the current one */
                        while(capStart > lineBreak) {
                            /* Increase the line number and reset the column offset */
                            ++line;
                            colOffset = lineBreak;
                            lineBreak = lineIndexes.front();
                            lineIndexes.pop_front();
                        }
                        WordList words;
                        /* Get the words from the extracted literal */
                        tokenizeWords(docPtr->fileName(), tokenString, 0, trUnit, words, false);
                        for(Word& word: words) {
                            /* Apply the offsets to the words */
                            word.columnNumber += capStart - colOffset;
                            word.lineNumber = line;
                        }
                        /* Apply settings */
                        applySettingsToWords(tokenString, words, false, wordsInSource);
                        /* The resulting words can be checked for spelling mistakes. */
                        parsedWords.append(words);
                    }
                }
            }
        }
    }

    if(d->settings->whatToCheck.testFlag(CppParserSettings::CheckComments) == true) {
        /* Parse comments */
        unsigned int commentCount = trUnit->commentCount();
        for(unsigned int comment = 0; comment < commentCount; ++comment) {
            const CPlusPlus::Token& token = trUnit->commentAt(comment);
            /* Check to see if the current comment type must be checked */
            if((d->settings->commentsToCheck.testFlag(CppParserSettings::CommentsC) == false)
                    && (token.kind() == CPlusPlus::T_COMMENT)) {
                /* C Style comments should not be checked and this is one */
                continue;
            }
            if((d->settings->commentsToCheck.testFlag(CppParserSettings::CommentsCpp) == false)
                    && (token.kind() == CPlusPlus::T_CPP_COMMENT)) {
                /* C++ Style comments should not be checked and this is one */
                continue;
            }

            bool isDoxygenComment = ((token.kind() == CPlusPlus::T_DOXY_COMMENT)
                                     || (token.kind() == CPlusPlus::T_CPP_DOXY_COMMENT));
            parseToken(docPtr, token, trUnit, wordsInSource, /* Comment */ true, isDoxygenComment, parsedWords, tokenHashesIn, tokenHashesOut);
        }
    }
    /* Move the new list of hashes to the member data so that
     * it can be used the next time around. Move is made explicit since
     * the LHS can be removed and the RHS will not be used again from
     * here on. */
    d->tokenHashes = std::move(tokenHashesOut);
    return parsedWords;
}
//--------------------------------------------------

void CppDocumentParser::parseToken(CPlusPlus::Document::Ptr docPtr, const CPlusPlus::Token& token, CPlusPlus::TranslationUnit *trUnit, const QSet<QString>& wordsInSource, bool isComment, bool isDoxygenComment, WordList& extractedWords, const HashWords &hashIn, HashWords &hashOut)
{
    WordList words;
    /* Get the token string */
    QString tokenString = QString::fromUtf8(docPtr->utf8Source().mid(token.bytesBegin(), token.bytes()).trimmed());
    /* Calculate the hash of the token string */
    quint32 hash = qHash(tokenString);
    /* Get the index of the token */
    quint32 commentBegin = token.utf16charsBegin();
    quint32 line;
    quint32 col;
    trUnit->getPosition(commentBegin, &line, &col);
    /* Search if the hash contains the given token. If it does
     * then the words that got extracted previously are used
     * as is, without attempting to extract them again. If the
     * token is not in the hash, it is a new token and must be
     * parsed to get the words from the token. */
    HashWords::const_iterator iter = hashIn.constFind(hash);
    const HashWords::const_iterator iterEnd = hashIn.constEnd();
    if(iter != iterEnd) {
        /* The token was parsed in a previous iteration.
         * Now check if the token moved due to lines being
         * added or removed. It it did not move, use the
         * words as is, if it did move, adjust the line and
         * column number of the words by the amount that the
         * token moved. */
        const TokenWords& tokenWords = (iter.value());
        if((tokenWords.line == line)
                && (tokenWords.col == col)) {
            /* Token did not move */
            extractedWords.append(tokenWords.words);
            hashOut[hash] = {line, col, tokenWords.words};
            return;
        } else {
            /* Token moved, adjust. */
            qint32 lineDiff = tokenWords.line - line;
            qint32 colDiff = tokenWords.col - col;
            for(Word word: tokenWords.words) {
                word.lineNumber   -= lineDiff;
                word.columnNumber -= colDiff;
                words.append(word);
            }
            hashOut[hash] = {line, col, words};
            extractedWords.append(words);
            return;
        }
    } else {
        /* Token was not in the list of hashes.
         * Tokenize the string to extract words that should be checked. */
        tokenizeWords(docPtr->fileName(), tokenString, commentBegin, trUnit, words, isComment);
        /* Apply the user settings to the words. */
        applySettingsToWords(tokenString, words, isDoxygenComment, wordsInSource);
        /* Check to see if there are words that should now be spell checked. */
        if(words.count() != 0) {
            extractedWords.append(words);
        }
        hashOut[hash] = {line, col, std::move(words)};
    }
    return;
}
//--------------------------------------------------

void CppDocumentParser::tokenizeWords(const QString& fileName, const QString &string, unsigned int stringStart, const CPlusPlus::TranslationUnit * const translationUnit, WordList &words, bool inComment)
{
    int strLength = string.length();
    bool busyWithWord = false;
    int wordStartPos = 0;
    bool endOfWord = false;

    /* Iterate through all of the characters in the comment and extract words from them.
     * Words are split up by non-word characters and is checked using the isEndOfCurrentWord()
     * function. The end condition is deliberately set to continue past the length of the
     * comment so that a word in progress is stopped and extracted when the comment ends */
    for(int currentPos = 0; currentPos <= strLength; ++currentPos) {
        /* Check if the current character is the end of a word character. */
        endOfWord = isEndOfCurrentWord(string, currentPos);
        if((endOfWord == false) && (busyWithWord == false)){
            wordStartPos = currentPos;
            busyWithWord = true;
        }

        if((busyWithWord == true) && (endOfWord == true)) {
            Word word;
            word.fileName = fileName;
            word.text = string.mid(wordStartPos, currentPos - wordStartPos);
            word.start = wordStartPos;
            word.end = currentPos;
            word.length = currentPos - wordStartPos;
            word.charAfter = (currentPos < strLength)? string.at(currentPos): QLatin1Char(' ');
            word.inComment = inComment;
            translationUnit->getPosition(stringStart + wordStartPos, &word.lineNumber, &word.columnNumber);
            words.append(word);
            busyWithWord = false;
            wordStartPos = 0;
        }
    }
    return;
}
//--------------------------------------------------

void CppDocumentParser::applySettingsToWords(const QString &string, WordList &words, bool isDoxygenComment, const QSet<QString> &wordsInSource)
{
    using namespace SpellChecker::Parsers::CppParser;

    /* Filter out words that appears in the source. They are checked against the list
     * of words parsed from the file before the for loop. */
    if(d->settings->removeWordsThatAppearInSource == true) {
        removeWordsThatAppearInSource(wordsInSource, words);
    }

    /* Regular Expressions that might be used, defined here so that it does not get cleared in the loop.
     * They are made static const because they will be re-used a lot and will never be changed. This way
     * the construction of the objects can be done once and then be re-used. */
    static const QRegularExpression doubleRe(QLatin1String("\\A\\d+(\\.\\d+)?\\z"));
    static const QRegularExpression hexRe(QLatin1String("\\A0x[0-9A-Fa-f]+\\z"));
    static const QRegularExpression emailRe(QLatin1String("\\A") + QLatin1String(SpellChecker::Parsers::CppParser::Constants::EMAIL_ADDRESS_REGEXP_PATTERN) + QLatin1String("\\z"));
    static const QRegularExpression websiteRe(QLatin1String("") + QLatin1String(SpellChecker::Parsers::CppParser::Constants::WEBSITE_ADDRESS_REGEXP_PATTERN));
    static const QRegularExpression websiteCharsRe(QLatin1String("") + QLatin1String(SpellChecker::Parsers::CppParser::Constants::WEBSITE_CHARS_REGEXP_PATTERN));
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

        /* Attempt to remove website addresses using the websiteRe Regular Expression. */
        if((d->settings->removeWebsites == true) && (removeCurrentWord == false)) {
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
                    applySettingsToWords(string, wordsFromSplit, isDoxygenComment, wordsInSource);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                }
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
            static const QRegularExpression numberContainRe(QLatin1String("[0-9]"));
            static const QRegularExpression numberSplitRe(QLatin1String("[0-9]+"));
            if(currentWord.contains(numberContainRe) == true) {
                /* Handle words with numbers based on the setting that is set for them */
                if(d->settings->wordsWithNumberOption == CppParserSettings::RemoveWordsWithNumbers) {
                    removeCurrentWord = true;
                } else if(d->settings->wordsWithNumberOption == CppParserSettings::SplitWordsOnNumbers) {
                    removeCurrentWord = true;
                    QStringList wordsSplitOnNumbers = currentWord.split(numberSplitRe, QString::SkipEmptyParts);
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnNumbers, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(string, wordsFromSplit, isDoxygenComment, wordsInSource);
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
                    static const QRegularExpression underscoreSplitRe(QLatin1String("_+"));
                    QStringList wordsSplitOnUnderScores = currentWord.split(underscoreSplitRe, QString::SkipEmptyParts);
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnUnderScores, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(string, wordsFromSplit, isDoxygenComment, wordsInSource);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    Q_ASSERT(false);
                }
            }
        }

        /* Settings for CamelCase */
        if((d->settings->camelCaseWordOption != CppParserSettings::LeaveWordsInCamelCase) && (removeCurrentWord == false)) {
            /* Check to see if the word appears to be in camelCase. If it does, handle according to the settings */
            /* The check is not precise and accurate science, but a rough estimation of the word is in camelCase. This
             * will probably be updated as this gets tested. The current check checks for one or more lower case letters,
             * followed by one or more upper-case letter, followed by a lower case letter */
            static const QRegularExpression camelCaseContainsRe(QLatin1String("[a-z]{1,}[A-Z]{1,}[a-z]{1,}"));
            static const QRegularExpression camelCaseIndexRe(QLatin1String("[a-z][A-Z]"));
            if(currentWord.contains(camelCaseContainsRe) == true ) {
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
                    applySettingsToWords(string, wordsFromSplit, isDoxygenComment, wordsInSource);
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
                    static const QRegularExpression dotsSplitRe(QLatin1String("\\.+"));
                    QStringList wordsSplitOnDots = currentWord.split(dotsSplitRe, QString::SkipEmptyParts);
                    WordList wordsFromSplit;
                    IDocumentParser::getWordsFromSplitString(wordsSplitOnDots, (*iter), wordsFromSplit);
                    /* Apply the settings to the words that came from the split to filter out words that does
                     * not belong due to settings. After they have passed the settings, add the words that survived
                     * to the list of words that should be added in the end */
                    applySettingsToWords(string, wordsFromSplit, isDoxygenComment, wordsInSource);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    Q_ASSERT(false);
                }
            }
        }

        /* Doxygen comments */
        if((isDoxygenComment == true) && (removeCurrentWord == false)) {
            if((string.at((*iter).start - 1) == QLatin1Char('\\'))
                    || (string.at((*iter).start - 1) == QLatin1Char('@'))) {
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

void CppDocumentParser::getWordsThatAppearInSource(CPlusPlus::Document::Ptr docPtr, QSet<QString> &wordsInSource)
{
    if(docPtr == nullptr) {
        Q_ASSERT(docPtr != nullptr);
        return;
    }
    unsigned total = docPtr->globalSymbolCount();
    CPlusPlus::Overview overview;
    for (unsigned i = 0; i < total; ++i) {
        CPlusPlus::Symbol *symbol = docPtr->globalSymbolAt(i);
        getListOfWordsFromSourceRecursive(wordsInSource, symbol, overview);
    }
}
//--------------------------------------------------

void CppDocumentParser::getListOfWordsFromSourceRecursive(QSet<QString> &words, const CPlusPlus::Symbol *symbol, const CPlusPlus::Overview &overview)
{
    /* Get the pretty name and type for the current symbol. This name is then split up into
     * different words that are added to the list of words that appear in the source */
    QString name = overview.prettyName(symbol->name()).trimmed();
    QString type = overview.prettyType(symbol->type()).trimmed();
    getPossibleNamesFromString(words, name);
    getPossibleNamesFromString(words, type);

    /* Go to the next level into the scope of the symbol and get the words from that level as well*/
    const CPlusPlus::Scope *scope = symbol->asScope();
    if (scope != nullptr) {
        CPlusPlus::Scope::iterator cur = scope->memberBegin();
        while (cur != scope->memberEnd()) {
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

void CppDocumentParser::getPossibleNamesFromString(QSet<QString> &words, const QString& string)
{
    static const QRegularExpression nameRegExp(QStringLiteral("(\\w+)"));
    QRegularExpressionMatchIterator i = nameRegExp.globalMatch(string);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        words << match.captured(1);
    }
}
//--------------------------------------------------

void CppDocumentParser::removeWordsThatAppearInSource(const QSet<QString> &wordsInSource, WordList &words)
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
    QString lastWordRemoved;
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

    /* Check if the current character is a letter, number or underscore.
     * Some settings might change what the end of a word actually is.
     * For some settings an underscore will be considered as the end of a word */
    if((currentChar.isLetterOrNumber() == true)
            || currentChar == QLatin1Char('_')) {
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
        static const QRegularExpression wordChars(QStringLiteral("\\w"));
        if((wordChars.match(comment.at(currentPos + 1)).hasMatch())
                &&(wordChars.match(comment.at(currentPos - 1)).hasMatch())) {
            return false;
        }
    }

    /* For word with @ in: Email address */
    if(currentChar == QLatin1Char('@')) {
        if((currentPos == 0) || (currentPos == (comment.length() - 1))) {
            return true;
        }
        static const QRegularExpression wordChars(QStringLiteral("\\w"));
        if((wordChars.match(comment.at(currentPos + 1)).hasMatch())
                &&(wordChars.match(comment.at(currentPos - 1)).hasMatch())) {
            return false;
        }
    }

    /* Check for websites. This will only check the website characters if the
     * option for website addresses are enabled due to the amount of false positives
     * that this setting can remove. Also this can put some overhead to other settings
     * that are not always desired.
     * This setting might require some rework in the future. */
    if(d->settings->removeWebsites == true) {
        static const QRegularExpression websiteChars(QStringLiteral("\\/|:|\\?|\\=|#|%|\\w|\\-"));
        if(websiteChars.match(currentChar).hasMatch() == true) {
            if((currentPos == 0) || (currentPos == (comment.length() - 1))) {
                return true;
            }
            if((websiteChars.match(comment.at(currentPos + 1)).hasMatch() == true)
                    &&(websiteChars.match(comment.at(currentPos - 1)).hasMatch() == true)) {
                return false;
            }
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
            if(word.toUpper() == QStringLiteral("CPP"))
                return true;
            break;
        }
    case 4:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'E':
            if(word.toUpper() == QStringLiteral("ENUM"))
                return true;
            break;
        }
    case 6:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'S':
            if(word.toUpper() == QStringLiteral("STRUCT"))
                return true;
            break;
        }
        break;
    case 7:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'D':
            if(word.toUpper() == QStringLiteral("DOXYGEN"))
                return true;
            break;
        case 'N':
            if(word.toUpper() == QStringLiteral("NULLPTR"))
                return true;
            break;
        case 'T':
            if(word.toUpper() == QStringLiteral("TYPEDEF"))
                return true;
            break;
        }
        break;
    case 9:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'N':
            if(word.toUpper() == QStringLiteral("NAMESPACE"))
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
