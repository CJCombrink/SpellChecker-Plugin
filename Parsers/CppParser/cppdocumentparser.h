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

protected:
    void setCurrentEditor(const QString& editorFilePath) Q_DECL_OVERRIDE;
    void setActiveProject(ProjectExplorer::Project* activeProject) Q_DECL_OVERRIDE;
    void updateProjectFiles(QStringSet filesAdded, QStringSet filesRemoved) Q_DECL_OVERRIDE;

protected slots:
    void parseCppDocumentOnUpdate(CPlusPlus::Document::Ptr docPtr);
    void settingsChanged();

public:

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
    /*! \brief Apply the user Settings to the Words.
     * \param[in] string String that these words belong to.
     * \param[inout] words words that should be parsed. Words will be removed from this list
     *                  based on the user settings.
     * \param[in] wordsInSource List of words that appear in the source. Based on the user
     *                  setting words that appear in this list will be removed from the
     *                  final list of \a words. */
    void applySettingsToWords(const QString& string, WordList& words, const QStringSet &wordsInSource);
    static bool isEndOfCurrentWord(const QString& comment, int currentPos, const CppParserSettings &settings);
    bool isReservedWord(const QString& word);

private:
    friend CppDocumentParserPrivate;
    CppDocumentParserPrivate* const d;
};

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker
