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

#pragma once

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
/*! \brief Hash of a token and the corresponding list of words that were extracted from the token.
 *
 * The quint32 is result of the qHash(QString, 0) and stored in the hash for each token, along with the
 * list of words that were extracted from that comment. The hash of the token is used instead of the string
 * because there is no need for the extra memory in the hash to store the actual token. If the hash was
 * defined as QHash<QString, CommentWords> the hash would store the full token in memory so that it can
 * be obtained using the QHash::key() function. Some token can be long and the overhead is not needed. */
using HashWords = QHash<quint32, TokenWords>;

class CppDocumentParser : public SpellChecker::IDocumentParser
{
    Q_OBJECT
public:
    CppDocumentParser(QObject *parent = nullptr);
    ~CppDocumentParser() Q_DECL_OVERRIDE;
    QString displayName() Q_DECL_OVERRIDE;
    Core::IOptionsPage* optionsPage() Q_DECL_OVERRIDE;
    void parseToken(QStringList wordsInSource, CPlusPlus::TranslationUnit *trUnit, WordList words, CPlusPlus::Document::Ptr docPtr, WordList parsedWords, const CPlusPlus::Token& token);

protected:
    void setCurrentEditor(const QString& editorFilePath) Q_DECL_OVERRIDE;
    void setActiveProject(ProjectExplorer::Project* activeProject) Q_DECL_OVERRIDE;
    void updateProjectFiles(QStringSet filesAdded, QStringSet filesRemoved) Q_DECL_OVERRIDE;

protected slots:
    void parseCppDocumentOnUpdate(CPlusPlus::Document::Ptr docPtr);
    void settingsChanged();

private:
    /*! \brief The Word Tokens structure
     *
     * This structure is returned by the parseToken() function and contains
     * the list of words that were extracted from a token (comment, literal,
     * etc.). The structure also contains the hash of the token so that it can
     * be checked when the same file is parsed again. There is no need to store
     * the actual token since the hash comparison should be good enough to
     * check for changes.
     *
     * The \a line and \a column are stored for the token so that if a token did not
     * change, but it moved, the words that came from that token can just be
     * moved as needed without the need to do any string processing and parsing.
     *
     * The \a newHash flag keeps track if the words were extracted in a
     * previous pass or not, meaning that they were already processed and does not
     * need to be processed further. */
    struct WordTokens {
      enum class Type {
        Comment = 0,
        Doxygen,
        Literal
      };

      HashWords::key_type hash;
      uint32_t line;
      uint32_t column;
      QString string;
      WordList words;
      bool newHash;
      Type type;
    };

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
     *
     * A hash is passed to the function that contains a hash and a set of words
     * that were previously extracted from a token with that hash. The function
     * uses that information to check if the token that must be processed now
     * was processed before (the new hash should match a hash in the map). If
     * a hash was processed before, then the parser can just re-use all the words
     * that came from the token previously without the need to do any string
     * processing on the token. This hash also contains information as to where
     * the token was the previous time, so that if it just moved the words can
     * be updated accordingly. Benchmarks showed that for large files or files
     * with a lot of tokens this had a big speedup. On smaller files the effect
     * was not as much but on smaller files this effect is negligible compared
     * to the speedup on large files.
     *
     * \param[in] docPtr Document pointer to the current document.
     * \param[in] token Translation Unit Token that should be split up into words that
     *              should be checked.
     * \param[in] trUnit Translation unit of the Document.
     * \param[in] type If the token is a Comment, Doxygen Documentation or a
     *              String Literal. This is captured to go along with the
     *              word so that the tables and displays upstream can indicate
     *              the difference between a comment and a literal. This gets
     *              forwarded to the tokenizeWords() function where it is used
     *              to extract words.
     * \param[in] hashIn The hash from the previous pass off the document to speed
     *              up the processing as discussed above.
     * \return WordTokens structure containing enough information to be useful to
     *              the caller. */
    WordTokens parseToken(CPlusPlus::Document::Ptr docPtr, CPlusPlus::TranslationUnit *trUnit, const CPlusPlus::Token& token, WordTokens::Type type, const HashWords& hashIn);
    /*! \brief Parse all macros in the document and extract string literals.
     *
     * Only macros that are functions and have arguments that are string literals
     * are considered. This is important since QStringLiteral is a macro, and
     * there are also other macros that takes in literals as arguments. */
    QVector<WordTokens> parseMacros(CPlusPlus::Document::Ptr docPtr, CPlusPlus::TranslationUnit *trUnit, const HashWords& hashIn);
    /*! \brief Tokenize Words from a string.
     *
     * This function takes a string, either a comment or a string literal and
     * breaks the string into words or tokens that should later be checked
     * for spelling mistakes.
     * \param[in] fileName Name of the file that the string belongs to.
     * \param[in] string String that must be broken up.
     * \param[in] stringStart Start of the string.
     * \param[in] translationUnit Translation unit belonging to the current document.
     * \param[in] type If the string is a Comment, Doxygen Documentation or a
     *              String Literal. If the string is Doxygen docs then the
     *              function will also try to remove doxygen tags from the words
     *              extracted. This reduce the number of words returned that
     *              gets handled later on, and it does not rely on a setting,
     *              it must be done always to remove noise.
     * \return Words that were extracted from the string. */
    WordList tokenizeWords(const QString &fileName, const QString& string, uint32_t stringStart, const CPlusPlus::TranslationUnit *const translationUnit, WordTokens::Type type);
    /*! \brief Apply the user Settings to the Words.
     * \param[in] string String that these words belong to.
     * \param[inout] words words that should be parsed. Words will be removed from this list
     *                  based on the user settings.
     * \param[in] wordsInSource List of words that appear in the source. Based on the user
     *                  setting words that appear in this list will be removed from the
     *                  final list of \a words. */
    void applySettingsToWords(const QString& string, WordList& words, const QStringSet &wordsInSource);
    QStringSet getWordsThatAppearInSource(CPlusPlus::Document::Ptr docPtr);
    QStringSet getListOfWordsFromSourceRecursive(const CPlusPlus::Symbol* symbol, const CPlusPlus::Overview& overview);
    QStringSet getPossibleNamesFromString(const QString &string);
    bool isEndOfCurrentWord(const QString& comment, int currentPos);
    bool isReservedWord(const QString& word);

private:
    friend CppDocumentParserPrivate;
    CppDocumentParserPrivate* const d;
};

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker
