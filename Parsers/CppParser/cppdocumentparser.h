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

signals:
    
protected:
    void setCurrentEditor(const QString& editorFilePath) Q_DECL_OVERRIDE;
    void setActiveProject(ProjectExplorer::Project* activeProject) Q_DECL_OVERRIDE;

protected slots:
    void parseCppDocumentOnUpdate(CPlusPlus::Document::Ptr docPtr);
    void settingsChanged();

protected:
    void reparseProject();
    bool shouldParseDocument(const QString& fileName);
    WordList parseCppDocument(CPlusPlus::Document::Ptr docPtr);
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
    void applySettingsToWords(const QString& string, WordList& words, bool isDoxygenComment, const QStringList &wordsInSource = QStringList());
    void getWordsThatAppearInSource(CPlusPlus::Document::Ptr docPtr, QStringList& wordsInSource);
    void getListOfWordsFromSourceRecursive(QStringList &words, const CPlusPlus::Symbol* symbol, const CPlusPlus::Overview& overview);
    QStringList getPossibleNamesFromString(const QString &string);
    void removeWordsThatAppearInSource(const QStringList& wordsInSource, WordList& words);
    bool isEndOfCurrentWord(const QString& comment, int currentPos);
    bool isReservedWord(const QString& word);

private:
    CppDocumentParserPrivate* const d;
};

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker

#endif // SPELLCHECKER_CPPSPELLCHECKER_CPPDOCUMENTPARSER_H
