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

#include <QDir>

#include <algorithm>

using namespace SpellChecker;
using namespace SpellChecker::Internal;

class SpellingMistakesPredicate {
public:
    explicit SpellingMistakesPredicate(Constants::MistakesModelColumn columnIndex, Qt::SortOrder order) :
        m_columnIndex(columnIndex),
        m_order(order) {}

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
        /* Can only sort to line, word and literal columns. */
        case Constants::MISTAKE_COLUMN_LITERAL:
            /* If both words are not in comments, the one in a comment
             * is always the greatest one. If both are either in a comment
             * or string literal then sort according to the line number. */
            if(word1.inComment != word2.inComment) {
                return word1.inComment;
            } else {
                /* Use this same predicate to sort but this time on the line number. */
                SpellingMistakesPredicate predicate(Constants::MISTAKE_COLUMN_LINE, m_order);
                return predicate(word1, word2);
            }
        case Constants::MISTAKE_COLUMN_WORD:
            return word1.text < word2.text;
        case Constants::MISTAKE_COLUMN_LINE:
            /* If the line numbers are not the same, return the one with the
             * highest line number. Otherwise if the line numbers are the same,
             * compare the column numbers to get the correct one. */
            if(word1.lineNumber != word2.lineNumber) {
                return word1.lineNumber < word2.lineNumber;
            } else {
                return word1.columnNumber < word2.columnNumber;
            }
        default:
            return false;
        }
    }

private:
    Constants::MistakesModelColumn m_columnIndex;
    Qt::SortOrder m_order;
};
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

class SpellChecker::Internal::SpellingMistakesModelPrivate {
public:
    QList<SpellChecker::Word> wordList;
    Constants::MistakesModelColumn sortColumn;
    Qt::SortOrder sortOrder;
    QDir projectDir;

    SpellingMistakesModelPrivate() :
        sortColumn(Constants::MISTAKE_COLUMN_LINE),
        sortOrder(Qt::AscendingOrder),
        projectDir()
    {}

};
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------


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

QModelIndex SpellingMistakesModel::indexOfWord(const Word &word) const
{
    int idx = d->wordList.indexOf(word);
    if(idx == -1) {
        /* The word was not found in the List, return the invalid index */
        return QModelIndex();
    }

    /* The word was found, get an index and return it */
    return createIndex(idx, 0);
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
    if((role == Qt::DecorationRole)
        && (index.column() == Constants::MISTAKE_COLUMN_LITERAL)) {
        /* Display the icon if the word is a string literal. */
        Word currentWord = d->wordList.at(index.row());
        if(currentWord.inComment == false) {
            return QIcon(QLatin1String(":/extensionsystem/images/ok.png"));
        } else {
            return QVariant();
        }
    }
    if((index.isValid() == false)
            || (role != Qt::DisplayRole))
        return QVariant();

    Word currentWord = d->wordList.at(index.row());
    switch(index.column()) {
    case Constants::MISTAKE_COLUMN_IDX:
        return index.row() + 1;
    case Constants::MISTAKE_COLUMN_WORD:
        return currentWord.text;
    case Constants::MISTAKE_COLUMN_SUGGESTIONS:
        return currentWord.suggestions.join(QLatin1String(", "));
    case Constants::MISTAKE_COLUMN_LITERAL:
        /* No text, only the image above. */
        return QVariant();
    case Constants::MISTAKE_COLUMN_LINE:
        return currentWord.lineNumber;
    case Constants::MISTAKE_COLUMN_COLUMN:
        return currentWord.columnNumber;
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

void SpellingMistakesModel::sort(int column, Qt::SortOrder order)
{
    bool shouldSort = false;
    switch (Constants::MistakesModelColumn(column)) {
        /* Can only sort to line, word and literal columns. */
        case Constants::MISTAKE_COLUMN_LITERAL:
            /* Fall through */
        case Constants::MISTAKE_COLUMN_WORD:
            /* Fall through */
        case Constants::MISTAKE_COLUMN_LINE:
            shouldSort = true;
            break;
        default:
            shouldSort = false;
            break;
    }

    if(shouldSort == false) {
        return;
    }
    beginResetModel();
    d->sortColumn = Constants::MistakesModelColumn(column);
    d->sortOrder = order;
    SpellingMistakesPredicate predicate(d->sortColumn, d->sortOrder);
    std::sort(d->wordList.begin(), d->wordList.end(), predicate);
    endResetModel();
    emit layoutChanged();
    emit mistakesUpdated();
}
//--------------------------------------------------

void SpellingMistakesModel::setActiveProject(ProjectExplorer::Project *activeProject)
{
    if(activeProject == nullptr) {
        /* Clear the directory path. This would mean that the
         * directory will point to the current application directory
         * but this should not be a problem. */
        d->projectDir.setPath(QLatin1String("."));
        return;
    }
    d->projectDir.setPath(activeProject->projectDirectory().toString());
    Q_ASSERT(d->projectDir.exists() == true);
}
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
