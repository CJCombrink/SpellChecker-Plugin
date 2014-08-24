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

#ifndef SPELLCHECKER_INTERNAL_PROJECTMISTAKESMODEL_H
#define SPELLCHECKER_INTERNAL_PROJECTMISTAKESMODEL_H

#include "Word.h"

#include <QAbstractItemModel>

namespace SpellChecker {
namespace Internal {

class ProjectMistakesModelPrivate;
/*! \brief The ProjectMistakesModel class
 *
 * This class is the model for all of the mistakes of the current project.
 */
class ProjectMistakesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Columns {
        COLUMN_FILE = Qt::UserRole,
        COLUMN_MISTAKES_TOTAL,
        COLUMN_FILEPATH,
        COLUMN_COUNT
    };

    ProjectMistakesModel();
    ~ProjectMistakesModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int indexOfFile(const QString& fileName) const;

    /*! \brief Insert Spelling Mistakes for the given file.
     *
     * If there are no misspelled words for the file, the file will get removed
     * from the model.
     * \param[in] fileName Name of the file that the words belong to.
     * \param[in] words Misspelled words for the file.
     */
    void insertSpellingMistakes(const QString &fileName, const WordList &words);
    /*! \brief Clears all Spelling Mistakes added to the model.
     *
     * This would normally be done when the startup project gets changed.
     */
    void clearAllSpellingMistakes();
    /*! \brief Get all mistakes for a file
     *
     * If the file does not have any mistakes associated with it, this function will
     * return a list of empty words.
     * \param[in] fileName Name of the file.
     * \return A list of misspelled words for the file.
     */
    WordList mistakesForFile(const QString& fileName) const;
    /*! \brief Remove all occurrences of the word.
     *
     * This function is used to remove all occurrences of the given word from
     * the model. This will happen when a word is either ignored or added to
     * improve the speed over re-parsing all files in the project.
     * \param[in] wordText Word that must be removed.
     */
    void removeAllOccurrences(const QString& wordText);
public slots:
    void fileSelected(const QModelIndex& index);
private:
    ProjectMistakesModelPrivate* const d;
};
//--------------------------------------------------

} // namespace Internal
} // namespace SpellChecker

#endif // SPELLCHECKER_INTERNAL_PROJECTMISTAKESMODEL_H
