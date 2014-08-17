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

#include "spellingmistakesmodel.h"
#include "Word.h"
#include "spellcheckerconstants.h"

class SpellChecker::Internal::SpellingMistakesModelPrivate {
public:
    QList<SpellChecker::Word> wordList;
    Constants::MistakesModelColumn sortColumn;
    Qt::SortOrder sortOrder;

    SpellingMistakesModelPrivate() :
        sortColumn(Constants::MISTAKE_COLUMN_LINE),
        sortOrder(Qt::AscendingOrder)
    {}

};
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

using namespace SpellChecker;
using namespace SpellChecker::Internal;

SpellingMistakesModel::SpellingMistakesModel(QObject *parent) :
    QAbstractTableModel(parent),
    d(new SpellingMistakesModelPrivate())
{
}
//--------------------------------------------------

SpellingMistakesModel::~SpellingMistakesModel()
{
    delete d;
}
//--------------------------------------------------

void SpellingMistakesModel::setCurrentSpellingMistakes(const SpellChecker::WordList& words)
{
    beginResetModel();
    d->wordList = words.values();
    sort(d->sortColumn, d->sortOrder);
    endResetModel();
    emit layoutChanged();
    emit mistakesUpdated();
}
//--------------------------------------------------

int SpellingMistakesModel::rowCount(const QModelIndex &parent) const
{
    /* Make sure the hierarchy is correct, there should only be one level */
    if(parent.isValid() == true)
        return 0;

    return d->wordList.count();
}
//--------------------------------------------------

int SpellingMistakesModel::columnCount(const QModelIndex &parent) const
{
    /* Make sure the hierarchy is correct, there should only be one level */
    if(parent.isValid() == true)
        return 0;

    return Constants::MISTAKE_COLUMN_COUNT;
}
//--------------------------------------------------

QVariant SpellingMistakesModel::data(const QModelIndex &index, int role) const
{
    if((index.isValid() == false)
            || (role != Qt::DisplayRole))
        return QVariant();

    Word currentWord = d->wordList.at(index.row());
    switch(index.column()) {
    case Constants::MISTAKE_COLUMN_IDX:
        return index.row() + 1;
    case Constants::MISTAKE_COLUMN_WORD:
        return currentWord.text;
    case Constants::MISTAKE_COLUMN_FILE:
        return currentWord.fileName;
    case Constants::MISTAKE_COLUMN_LINE:
        return currentWord.lineNumber;
    case Constants::MISTAKE_COLUMN_COLUMN:
        return currentWord.columnNumber;
    case Constants::MISTAKE_COLUMN_SUGGESTIONS:
        return currentWord.suggestions.join(QLatin1String(", "));
    default:
        return QVariant();
    }
    return QVariant();
}
//--------------------------------------------------

QVariant SpellingMistakesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((orientation == Qt::Vertical)
            || (role != Qt::DisplayRole)
            || (section >= Constants::MISTAKE_COLUMN_COUNT))
        return QVariant();

    return tr(Constants::MISTAKES_MODEL_COLUMN_NAMES[section]);
}
//--------------------------------------------------


class SpellingMistakesPredicate
{
public:
    explicit SpellingMistakesPredicate(Constants::MistakesModelColumn columnIndex, Qt::SortOrder order) :
        m_columnIndex(columnIndex),
        m_order(order)
    {}

    inline bool operator()(const Word &word1, const Word &word2)
    {
        if (m_order == Qt::AscendingOrder)
            return lessThan(word1, word2);
        else
            return lessThan(word2, word1);
    }

    inline bool lessThan(const Word &word1, const Word &word2)
    {
        switch (m_columnIndex) {
            /* can only sort to column and word */
            case Constants::MISTAKE_COLUMN_WORD:
                return word1.text < word2.text;
            case Constants::MISTAKE_COLUMN_LINE:
                return word1.lineNumber < word2.lineNumber;
            default:
                return false;
        }
    }

private:
    Constants::MistakesModelColumn m_columnIndex;
    Qt::SortOrder m_order;
};

void SpellingMistakesModel::sort(int column, Qt::SortOrder order)
{
    bool shouldSort = false;
    switch (Constants::MistakesModelColumn(column)) {
        /* can only sort to column and word */
        case Constants::MISTAKE_COLUMN_WORD:
            /* Fall through */
        case Constants::MISTAKE_COLUMN_LINE:
            shouldSort = true;
            break;
        default:
            shouldSort = false;
    }

    if(shouldSort == false) {
        return;
    }
    d->sortColumn = Constants::MistakesModelColumn(column);
    d->sortOrder = order;

    SpellingMistakesPredicate predicate(d->sortColumn, d->sortOrder);
    qSort(d->wordList.begin(), d->wordList.end(), predicate);
    emit layoutChanged();
    emit mistakesUpdated();
}
//--------------------------------------------------