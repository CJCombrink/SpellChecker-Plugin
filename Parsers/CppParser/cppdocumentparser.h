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

    FileWordList parseFiles(const QStringList& fileNames);
    WordList parseFile(const QString& fileName);
    QString displayName();

    Core::IOptionsPage* optionsPage();
    void setStartupProject(ProjectExplorer::Project* startupProject);
    void setCurrentEditor(Core::IEditor *editor);

signals:
    void addWordsWithSpellingMistakes(const QString& fileName, const SpellChecker::WordList& words);
    
public slots:

protected slots:
    void parseCppDocumentOnUpdate(CPlusPlus::Document::Ptr docPtr);
    void settingsChanged();

protected:
    void reparseProject();
    bool shouldParseDocument(const QString& fileName);
    WordList parseAndSpellCheckCppDocument(CPlusPlus::Document::Ptr docPtr);
    void tokenizeWords(const QString &fileName, const QString& comment, unsigned int commentStart, const CPlusPlus::TranslationUnit *const translationUnit, WordList& words);
    void applySettingsToWords(const QString& comment, WordList& words, bool isDoxygenComment);
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
