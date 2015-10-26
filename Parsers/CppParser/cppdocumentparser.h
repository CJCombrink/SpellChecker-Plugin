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

#ifndef SPELLCHECKER_CPPSPELLCHECKER_CPPDOCUMENTPARSER_H
#define SPELLCHECKER_CPPSPELLCHECKER_CPPDOCUMENTPARSER_H

#include "../../idocumentparser.h"

#include <cplusplus/CppDocument.h>
#include <projectexplorer/projectexplorer.h>

#include <QObject>

namespace CPlusPlus { class Overview; }

namespace SpellChecker {
namespace CppSpellChecker {
namespace Internal {

class CppParserSettings;
class CppDocumentParserPrivate;

class CppDocumentParser : public SpellChecker::IDocumentParser
{
    Q_OBJECT
public:
    CppDocumentParser(QObject *parent = 0);
    virtual ~CppDocumentParser();
    QString displayName();
    Core::IOptionsPage* optionsPage();

    void parseToken(QStringList wordsInSource, CPlusPlus::TranslationUnit *trUnit, WordList words, CPlusPlus::Document::Ptr docPtr, WordList parsedWords, const CPlusPlus::Token& token);
signals:
    
protected:
    void setCurrentEditor(const QString& editorFilePath) Q_DECL_OVERRIDE;
    void setActiveProject(ProjectExplorer::Project* activeProject) Q_DECL_OVERRIDE;

protected slots:
    void parseCppDocumentOnUpdate(CPlusPlus::Document::Ptr docPtr);
    void settingsChanged();

public:
    void reparseProject();
    /*! \brief Query if the given file should be parsed.
     *
     * This function takes the global/core settings, local C++ Parser settings
     * as well as other plugin specific factors into account to determine if the
     * document should be parsed.
     * \param[in] fileName Name of the file that should be checked.
     * \return  true if the document should be parsed, otherwise false. */
    bool shouldParseDocument(const QString& fileName);
    /*! \brief Parse the Cpp Document.
     *
     * Work through the document and parse the file to extract words that should
     * be checked for spelling mistakes.
     * \param[in] docPtr Pointer to the document that will get parsed.
     * \return A list of words extracted that should be checked for spelling mistakes. */
    WordList parseCppDocument(CPlusPlus::Document::Ptr docPtr);
    /*! \brief Parse a Token retrieved from the Translation Unit of the document.
     *
     * Since both Comments and String Literals are tokens, the common code to extract the
     * words was added to a function to only work on a token.
     * \param[in] docPtr Document pointer to the current document.
     * \param[in] token Translation Unit Token that should be split up into words that
     *              should be checked.
     * \param[in] trUnit Translation unit of the Document.
     * \param[in] wordsInSource Words that appear in the source of the project.
     * \param[in] isComment If the token is a Comment or a String Literal.
     * \param[in] isDoxygenComment If the token is a Comment, if it is a normal or
     *              doxygen comment.
     * \param[out] extractedWords Words that were extracted from the token that must now
     *              be spellchecked. The extracted words will only get added to the list
     *              and previous items added will stay in the list. */
    void parseToken(CPlusPlus::Document::Ptr docPtr, const CPlusPlus::Token& token, CPlusPlus::TranslationUnit *trUnit, const QSet<QString> &wordsInSource, bool isComment, bool isDoxygenComment, WordList& extractedWords);
    /*! \brief Tokenize Words from a string.
     *
     * This function takes a string, either a comment or a string literal and
     * breaks the string into words or tokens that should later be checked
     * for spelling mistakes.
     * \param[in] fileName Name of the file that the string belongs to.
     * \param[in] string String that must be broken up.
     * \param[in] stringStart Start of the string.
     * \param[in] translationUnit Translation unit belonging to the current document.
     * \param[out] words Words that were extracted from the string.
     * \param[in] inComment If the string is a comment or a String Literal. */
    void tokenizeWords(const QString &fileName, const QString& string, unsigned int stringStart, const CPlusPlus::TranslationUnit *const translationUnit, WordList& words, bool inComment);
    /*! \brief Apply the user Settings to the Words.
     * \param[in] string String that these words belong to.
     * \param[inout] words words that should be parsed. Words will be removed from this list
     *                  based on the user settings.
     * \param[in] isDoxygenComment If the word is part of a doxygen comment.
     * \param[in] wordsInSource List of words that appear in the source. Based on the user
     *                  setting words that appear in this list will be removed from the
     *                  final list of \a words. */
    void applySettingsToWords(const QString& string, WordList& words, bool isDoxygenComment, const QSet<QString> &wordsInSource = QSet<QString>());
    void getWordsThatAppearInSource(CPlusPlus::Document::Ptr docPtr, QSet<QString>& wordsInSource);
    void getListOfWordsFromSourceRecursive(QSet<QString> &words, const CPlusPlus::Symbol* symbol, const CPlusPlus::Overview& overview);
    void getPossibleNamesFromString(QSet<QString> &words, const QString &string);
    void removeWordsThatAppearInSource(const QSet<QString> &wordsInSource, WordList& words);
    bool isEndOfCurrentWord(const QString& comment, int currentPos);
    bool isReservedWord(const QString& word);

private:
    CppDocumentParserPrivate* const d;
};

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker

#endif // SPELLCHECKER_CPPSPELLCHECKER_CPPDOCUMENTPARSER_H
