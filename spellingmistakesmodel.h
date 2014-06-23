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

#ifndef SPELLINGMISTAKESMODEL_H
#define SPELLINGMISTAKESMODEL_H

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

    void setCurrentSpellingMistakes(const WordList &words);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent  = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    void sort(int column, Qt::SortOrder order);
    
signals:
    void mistakesUpdated();
    
public slots:
private:
    SpellingMistakesModelPrivate* const d;
    
};
} // namespace Internal
} // namespace SpellChecker

#endif // SPELLINGMISTAKESMODEL_H
