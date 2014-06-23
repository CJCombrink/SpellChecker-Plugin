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

#ifndef SPELLCHECKER_IDOCUMENTPARSER_H
#define SPELLCHECKER_IDOCUMENTPARSER_H

#include "Word.h"

#include <projectexplorer/projectexplorer.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QObject>

namespace Core { class IOptionsPage; }

namespace SpellChecker {


/*! \brief The IDocumentParser class
 *
 * Parse a document and return all words that must be checked for
 * spelling mistakes.
 *
 * The document parsers must implement the functionality to check a list of files
 * as well as a single file. For the
 */
class IDocumentParser : public QObject
{
    Q_OBJECT
public:
    IDocumentParser(QObject *parent = 0);
    virtual ~IDocumentParser();

    /*!
     * \brief parse the list of files and return the words found in each file.
     *
     * This function takes a list of files to parse. For each file it should then
     * parse the file and get a list of the words that should be checked. This must
     * be done for each file in the list and then the words must be combined into a
     * list and returned to the caller.
     *
     * The base implementation iterates through the list of files and then calls
     * parseFile() on each of the files in the list. This can be changed if there
     * is a more optimised way to do this.
     *
     * \param fileNames List of file names that should be parsed.
     * \return A list containing the file names along with all potential spelling
     *          words that were found in that file.
     * \sa parseFile()
     */
    virtual FileWordList parseFiles(const QStringList& fileNames);
    /*! \brief Parse an individual file and get potential words from it.
     *
     * While the parseFiles() function checks a list of files, this function
     * only checks a single file and returns the list of words that must be checked
     * in that function.
     *
     *
     * \sa parseFiles() for some optimisation options around this file.
     *
     * \param fileName Name of the file that must be parsed.
     * \return List of words that must be checked by the spell checker.
     */
    virtual WordList parseFile(const QString& fileName) = 0;

    virtual QString displayName() = 0;

    virtual Core::IOptionsPage* optionsPage() = 0;

    virtual void setStartupProject(ProjectExplorer::Project* startupProject) = 0;
    virtual void setCurrentEditor(Core::IEditor *editor) = 0;
    
protected:
    void getWordsFromSplitString(const QStringList& stringList, const Word& word, WordList& wordList);
signals:
    
public slots:
    
};

} // namespace SpellChecker

#endif // SPELLCHECKER_IDOCUMENTPARSER_H
