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

#include <projectexplorer/project.h>

#include "Word.h"

#include <QAbstractTableModel>

namespace SpellChecker {
namespace Internal {

class SpellingMistakesModelPrivate;
class SpellingMistakesModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SpellingMistakesModel(QObject *parent = 0);
    ~SpellingMistakesModel();

    /*! \ brief Set the words of the model.
     *
     * This function clears the previous words and sets the model
     * to contain the new \a words. This will then be reflected on
     * all views connected to the model.
     * \param[in] words List of words that must be set on the model. */
    void setCurrentSpellingMistakes(const WordList &words);
    /*! \brief Get the index of the word.
     *
     * Get the index of the \a word from the model.
     * If the word is not in the list of words on this model then
     * this function will return an invalid index.
     * \param[in] word Word of which the index must be returned.
     * \return Index of the given word. */
    QModelIndex indexOfWord(const Word& word) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent  = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    void sort(int column, Qt::SortOrder order);

signals:
    void mistakesUpdated();
public slots:
    /*! \brief Slot called when the active project changes.
     *
     * The path to the active project is used by the model to
     * show the relative paths to the files with the spelling mistakes.
     * \param[in] activeProject Pointer to the active project. */
    void setActiveProject(ProjectExplorer::Project* activeProject);
private:
    SpellingMistakesModelPrivate* const d;
};

} // namespace Internal
} // namespace SpellChecker
