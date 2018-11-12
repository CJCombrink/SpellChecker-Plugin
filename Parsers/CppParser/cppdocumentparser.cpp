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
#include <utils/mimetypes/mimedatabase.h>

#include <QRegularExpression>
#include <QTextBlock>
#include <QApplication>


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


class CppDocumentParserPrivate {
public:
    ProjectExplorer::Project* activeProject;
    QString currentEditorFileName;
    CppParserOptionsPage* optionsPage;
    CppParserSettings* settings;
    QStringSet filesInStartupProject;

    HashWords tokenHashes;

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
     * custom MIME Type added by this plygin. */
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

    using TmpOptional = std::pair<bool, CppDocumentParser::WordTokens>;
    static TmpOptional checkHash(CppDocumentParser::WordTokens tokens, uint32_t hash, const HashWords& hashIn)
    {
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
          if((tokenWords.line == tokens.line)
                  && (tokenWords.col == tokens.column)) {
              tokens.words   = tokenWords.words;
              tokens.newHash = false;
              return std::make_pair(true, tokens);
          } else {
              WordList words;
              /* Token moved, adjust.
               * This will even work for lines that are copied because the
               * hash will be the same but the start will just be different. */
              const qint32 lineDiff = int32_t(tokenWords.line) - int32_t(tokens.line);
              const qint32 colDiff = int32_t(tokenWords.col) - int32_t(tokens.column);
              const uint32_t firstLine = tokenWords.words.at(0).lineNumber - lineDiff;
              for(Word word: qAsConst(tokenWords.words)) {
                  word.lineNumber   -= lineDiff;
                  if(word.lineNumber == tokenWords.line) {
                    word.columnNumber -= colDiff;
                  }
                  words.append(word);
              }
              tokens.words   = words;
              tokens.newHash = false;
              qDebug() << "STR: " << tokens.string;
              return std::make_pair(true, tokens);
          }
      }
      return std::make_pair(false, CppDocumentParser::WordTokens{});
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
    if(d->activeProject == nullptr) {
        return;
    }
    reparseProject();
}
//--------------------------------------------------

void CppDocumentParser::updateProjectFiles(QStringSet filesAdded, QStringSet filesRemoved)
{
  Q_UNUSED(filesRemoved)
  /* Only re-parse the files that were added. */
  CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

  const QStringSet fileSet = d->getCppFiles(filesAdded);
  d->filesInStartupProject.unite(fileSet);
  modelManager->updateSourceFiles(fileSet);
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

    const Utils::FileNameList projectFiles = d->activeProject->files(ProjectExplorer::Project::SourceFiles);
    const QStringList fileList = Utils::transform(projectFiles, &Utils::FileName::toString);

    const QStringSet fileSet = d->getCppFiles(fileList.toSet());
    d->filesInStartupProject = fileSet;
    modelManager->updateSourceFiles(fileSet);
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

    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    auto snap = modelManager->snapshot();

    if((settings->checkExternalFiles) == false) {
        /* Do not check external files so check if the file is part of the
         * active project. */
        return d->filesInStartupProject.contains(fileName);
    }

    return true;
}
//--------------------------------------------------

QVector<CppDocumentParser::WordTokens> CppDocumentParser::parseMacros(CPlusPlus::Document::Ptr docPtr, CPlusPlus::TranslationUnit *trUnit, const HashWords &hashIn)
{
  /* Get the macros from the document pointer. The arguments of the macro will then be parsed
   * and checked for spelling mistakes.
   * Since the TranslationUnit::getPosition() is not usable with the Macro Uses a manual method to extract
   * the literals and their line and column positions are implemented. This involves getting the unprocessed
   * source from the document for the macro and tracking the byte offsets manually from there.
   * An alternative implementation can involve using the Snapshot::preprocessedDocument(), which makes use of the FastPreprocessor
   * and the AST visitors.
   * For more information around a discussion on the mailing list around this issue refer to the following
   * link: http://comments.gmane.org/gmane.comp.lib.qt.creator/11853 */
    QVector<WordTokens> tokenizedWords;
    QList<CPlusPlus::Document::MacroUse> macroUse = docPtr->macroUses();
    if(macroUse.count() == 0) {
      return {};
    }
    static CppTools::CppModelManager *cppModelManager = CppTools::CppModelManager::instance();
    CppTools::CppEditorDocumentHandle *cppEditorDocument = cppModelManager->cppEditorDocument(docPtr->fileName());
    if(cppEditorDocument == nullptr) {
      return {};
    }
    const QByteArray source = cppEditorDocument->contents();

    for(const CPlusPlus::Document::MacroUse& mac: macroUse) {
        if(mac.isFunctionLike() == false) {
          /* Only macros that are function like should be considered.
           * These are ones that have arguments, that can be literals.
           */
          continue;
        }
        const QVector<CPlusPlus::Document::Block>& args = mac.arguments();
        if(args.count() == 0) {
            /* There are no arguments for the macro, no need to consider
             * further. */
            continue;
        }
        /* The line number of the macro is used as the reference. */
        uint32_t line  = mac.beginLine();
        /* Get the start of the line from the source. From this start the offset to the
         * words will be calculated. */
        const int32_t start = source.lastIndexOf('\n', int32_t(mac.utf16charsBegin()));
        /* Get the end index of the last argument of the macro. */
        const int32_t end   = int32_t(args.last().utf16charsEnd());
        if(end == 0) {
            /* This will happen on an empty argument, for example Q_ASSERT() */
            continue;
        }

        /* Get the full macro from the start of the line until the last byte of the last
         * argument. */
        const QByteArray macroBytes = source.mid(start, end - start);
        /* Quick check to see if there are any string literals in the macro text.
         * if the are this check can be a waist, but if not this can speed up the check by
         * avoiding an unneeded regular expression. */
        if(macroBytes.contains('\"') == false) {
            continue;
        }

        /* Check if the hash of the macro is not already contained
         * in the list of known hashes. Since the macroBytes contains
         * the data from the start of the line, any update in the line
         *
         */
        const uint32_t hash = qHash(macroBytes.mid(mac.utf16charsBegin() - start));
        qDebug() << "MAC: " << mac.macro().name()
                 << "\n   - " << macroBytes.mid(mac.utf16charsBegin() - start);
        WordTokens tokens;
        tokens.hash   = hash;
        tokens.column = mac.utf16charsBegin() - start;
        tokens.line   = line;
        tokens.string = QString::fromUtf8(macroBytes.mid(mac.utf16charsBegin() - start));
        tokens.type   = WordTokens::Type::Literal;

        CppDocumentParserPrivate::TmpOptional wordOpt = CppDocumentParserPrivate::checkHash(tokens, hash, hashIn);
        if(wordOpt.first == true) {
           tokenizedWords.append(wordOpt.second);
           continue;
        }

        /* Get the byte offsets inside the macro bytes for each line break inside the macro.
         * This will be used during the extraction to get the correct lines relative to the
         * line that the macro started on. */
        QVector<uint32_t> lineIndexes;
        /* The search above for the start include the new line character so the start for other
         * new lines must start from index 1 so that it is not also included. */
        int32_t startSearch = 1;
        while(true) {
            int idx = macroBytes.indexOf('\n', startSearch);
            if(idx < 0) {
                /* No more new lines, break out */
                break;
            }
            lineIndexes << uint32_t(idx);
            /* Must increment the start of the next search otherwise it will be found on
             * that index. */
            startSearch = idx + 1;
        }
        /* The end is also added to simplify some of the checks further on. No character can
         * fall outside of the end. */
        lineIndexes << uint32_t(end);
        uint32_t lineBreak = lineIndexes.takeFirst();
        uint32_t colOffset = 0;

        /* Use a regular expression to get all string literals from the macro and its arguments. */
        static const QRegularExpression regExp(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""));
        QRegularExpressionMatchIterator regExpIter = regExp.globalMatch(QString::fromLatin1(macroBytes));
        while(regExpIter.hasNext() == true) {
            const QRegularExpressionMatch match = regExpIter.next();
            const QString tokenString           = match.captured(0);
            Q_ASSERT(match.capturedStart(0) >= 0);
            const uint32_t capStart = uint32_t(match.capturedStart(0));
            /* Check if the literal starts on the next line from the current one */
            while(capStart > lineBreak) {
                /* Increase the line number and reset the column offset */
                ++line;
                colOffset = lineBreak;
                lineBreak = lineIndexes.takeFirst();
            }
            /* Get the words from the extracted literal */
            WordList words = tokenizeWords(docPtr->fileName(), tokenString, 0, trUnit, WordTokens::Type::Literal);
            for(Word& word: words) {
                /* Apply the offsets to the words */
                word.columnNumber += capStart - colOffset;
                word.lineNumber    = line;
            }
            tokens.words.append(words);
            /* Get the words from the extracted literal */
        }
        if(tokens.words.count() != 0) {
          tokenizedWords.append(tokens);
        }
    }
    return tokenizedWords;
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

    QVector<WordTokens> tokenizedWords;
    QStringSet wordsInSource;
    /* If the setting is set to remove words from the list based on words found in the source,
     * parse the source file and then remove all words found in the source files from the list
     * of words that will be checked. */
    if(d->settings->removeWordsThatAppearInSource == true) {
        /* First get all words that does appear in the current source file. These words only
         * include variables and their types */
        wordsInSource = getWordsThatAppearInSource(docPtr);
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
                WordTokens tokens = parseToken(docPtr, trUnit, token, WordTokens::Type::Literal, tokenHashesIn);
                tokenizedWords.append(tokens);
            }
        }
        /* Parse macros */
        tokenizedWords += parseMacros(docPtr, trUnit, tokenHashesIn);
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

            WordTokens::Type type = WordTokens::Type::Comment;
            if((token.kind() == CPlusPlus::T_DOXY_COMMENT)
               || (token.kind() == CPlusPlus::T_CPP_DOXY_COMMENT)) {
              type = WordTokens::Type::Doxygen;
            }
            const WordTokens tokens = parseToken(docPtr, trUnit, token, type, tokenHashesIn);
            tokenizedWords.append(tokens);
        }
    }


    /* Populate the list of hashes from the tokens that was processed. */
    HashWords newHashesOut;
    WordList newSettingsApplied;
    for(const WordTokens& token: qAsConst(tokenizedWords)) {
      WordList words = token.words;
      if(token.newHash == true) {
        /* The words are new, they were not known in a previous hash
         * thus the settings must now be applied.
         * Only words that have already been checked against the settings
         * gets added to the hash, thus there is no need to apply the settings
         * again, since this will only waste time. */
        applySettingsToWords(token.string, words, wordsInSource);
      }
      newSettingsApplied.append(words);
      /* TODO: handle macros here as well. */
      if(token.hash != 0x00) {
          newHashesOut[token.hash] = {token.line, token.column, words};
      }
    }
    /* Move the new list of hashes to the member data so that
     * it can be used the next time around. Move is made explicit since
     * the LHS can be removed and the RHS will not be used again from
     * here on. */
    d->tokenHashes = std::move(newHashesOut);

    return newSettingsApplied;
}
//--------------------------------------------------

CppDocumentParser::WordTokens CppDocumentParser::parseToken(CPlusPlus::Document::Ptr docPtr, CPlusPlus::TranslationUnit *trUnit, const CPlusPlus::Token& token,WordTokens::Type type, const HashWords &hashIn)
{
    /* Get the token string */
    const QString tokenString = QString::fromUtf8(docPtr->utf8Source().mid(token.bytesBegin(), token.bytes()).trimmed());
    /* Calculate the hash of the token string */
    const uint32_t hash = qHash(tokenString);
    /* Get the index of the token */
    const uint32_t commentBegin = token.utf16charsBegin();
    uint32_t line;
    uint32_t col;
    trUnit->getPosition(commentBegin, &line, &col);
    /* Set up the known parts of the return structure.
     * The rest will be populated as needed below. */
    WordTokens tokens;
    tokens.hash   = hash;
    tokens.column = col;
    tokens.line   = line;
    tokens.string = tokenString;
    tokens.type   = type;

    CppDocumentParserPrivate::TmpOptional wordOpt = CppDocumentParserPrivate::checkHash(tokens, hash, hashIn);
    if(wordOpt.first == true) {
      return wordOpt.second;
    }

    /* Token was not in the list of hashes.
     * Tokenize the string to extract words that should be checked. */
    tokens.words         = tokenizeWords(docPtr->fileName(), tokenString, commentBegin, trUnit, type);
    tokens.newHash       = true;
    return tokens;
}
//--------------------------------------------------

WordList CppDocumentParser::tokenizeWords(const QString& fileName, const QString &string, uint32_t stringStart, const CPlusPlus::TranslationUnit * const translationUnit, WordTokens::Type type)
{
    WordList wordTokens;
    const int32_t strLength = string.length();
    bool busyWithWord       = false;
    int32_t wordStartPos    = 0;
    bool endOfWord          = false;

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
            /* Pre-condition sanity checks for debugging. The wordStartPos
             * can not be 0 or negative. A comment or literal always starts
             * with either a slash-star or slash-slash (comment) or inverted
             * comma (literal), thus there must always be something else before
             * the word starts.
             *
             * currentPos on the other hand can be at the end of the string
             * for example with a single line comment (slash-slash).
             */
            Q_ASSERT(wordStartPos > 0);
            Word word;
            word.fileName  = fileName;
            word.text      = string.mid(wordStartPos, currentPos - wordStartPos);
            word.start     = wordStartPos;
            word.end       = currentPos;
            word.length    = currentPos - wordStartPos;
            word.charAfter = (currentPos < strLength)? string.at(currentPos): QLatin1Char(' ');
            word.inComment = (type != WordTokens::Type::Literal);
            bool isDoxygenTag = false;
            if(type == WordTokens::Type::Doxygen) {
                const QChar charBeforeStart = string.at(wordStartPos - 1);
                if((charBeforeStart == QLatin1Char('\\'))
                   || (charBeforeStart == QLatin1Char('@'))) {
                    const QString currentWord = word.text;
                      /* Classify it */
                      const int32_t doxyClass = CppTools::classifyDoxygenTag(currentWord.unicode(), currentWord.size());
                      if(doxyClass != CppTools::T_DOXY_IDENTIFIER) {
                        /* It is a doxygen tag, mark it as such so that it does not end up
                         * in the list of words from this string. */
                        isDoxygenTag = true;
                    }
                }
            }
            if(isDoxygenTag == false) {
                translationUnit->getPosition(stringStart + uint32_t(wordStartPos), &word.lineNumber, &word.columnNumber);
                wordTokens.append(std::move(word));
            }
            busyWithWord = false;
            wordStartPos = 0;
        }
    }
    return wordTokens;
}
//--------------------------------------------------

void CppDocumentParser::applySettingsToWords(const QString &string, WordList &words, const QStringSet &wordsInSource)
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
                    applySettingsToWords(string, wordsFromSplit, wordsInSource);
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
                    applySettingsToWords(string, wordsFromSplit, wordsInSource);
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
                    applySettingsToWords(string, wordsFromSplit, wordsInSource);
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
                    applySettingsToWords(string, wordsFromSplit, wordsInSource);
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
                    applySettingsToWords(string, wordsFromSplit, wordsInSource);
                    wordsToAddInTheEnd.append(wordsFromSplit);
                } else {
                    Q_ASSERT(false);
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

QStringSet CppDocumentParser::getWordsThatAppearInSource(CPlusPlus::Document::Ptr docPtr)
{
    QStringSet wordsSet;
    Q_ASSERT(docPtr != nullptr);
    const uint32_t total = docPtr->globalSymbolCount();
    CPlusPlus::Overview overview;
    for (uint32_t i = 0; i < total; ++i) {
        CPlusPlus::Symbol *symbol = docPtr->globalSymbolAt(i);
        wordsSet += getListOfWordsFromSourceRecursive(symbol, overview);
    }
    return wordsSet;
}
//--------------------------------------------------

QStringSet CppDocumentParser::getListOfWordsFromSourceRecursive(const CPlusPlus::Symbol *symbol, const CPlusPlus::Overview &overview)
{
    QStringSet wordsInSource;
    /* Get the pretty name and type for the current symbol. This name is then split up into
     * different words that are added to the list of words that appear in the source */
    const QString name = overview.prettyName(symbol->name()).trimmed();
    const QString type = overview.prettyType(symbol->type()).trimmed();
    wordsInSource += getPossibleNamesFromString(name);
    wordsInSource += getPossibleNamesFromString(type);

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
            wordsInSource += getListOfWordsFromSourceRecursive(curSymbol, overview);
        }
    }
    return wordsInSource;
}
//--------------------------------------------------

QStringSet CppDocumentParser::getPossibleNamesFromString(const QString& string)
{
    QStringSet wordSet;
    static const QRegularExpression nameRegExp(QStringLiteral("(\\w+)"));
    QRegularExpressionMatchIterator i = nameRegExp.globalMatch(string);
    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();
        wordSet += match.captured(1);
    }
    return wordSet;
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
        case 'S':
            if(word.toUpper() == QStringLiteral("STD"))
                return true;
            break;
        }
        break;
    case 4:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'E':
            if(word.toUpper() == QStringLiteral("ENUM"))
                return true;
            break;
        }
        break;
    case 6:
        switch(word.at(0).toUpper().toLatin1()) {
        case 'S':
            if(word.toUpper() == QStringLiteral("STRUCT"))
                return true;
            break;
        case 'P':
            if(word.toUpper() == QStringLiteral("PLUGIN"))
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
