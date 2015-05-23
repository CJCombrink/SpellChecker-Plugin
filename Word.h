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

#ifndef WORD_H
#define WORD_H

#include <QString>
#include <QMultiHash>
#include <QDebug>
#include <QStringList>


namespace SpellChecker {

/*! \brief The Word class
 *
 * This class is a structure representing a word parsed from the source. It
 * contains information about the word, where it is located and suggestions
 * for the word if it was misspelled.
 */
class Word {
public:
    Word() {}
    ~Word() {}

    int start;
    int end;
    int length;

    unsigned int lineNumber;
    unsigned int columnNumber;
    QString text;
    QString fileName;
    QChar charAfter; /*!< Next character after the end of the word in the comment. */
    bool inComment; /*!< If the word comes from a comment or a String Literal. */
    QStringList suggestions;

    bool operator==(const Word &other) const {
        return ((lineNumber      == other.lineNumber)
                && (columnNumber == other.columnNumber)
                && (text         == other.text)
                && (fileName     == other.fileName));
    }
};

/* Define used to use different types of word lists
 * for the spell checker. The 2 different types were defined
 * to test usage as well speed/performance of different
 * implementations. This was not done yet but might still be
 * done in future. */
#define USE_MULTI_HASH
#ifdef USE_MULTI_HASH
class WordList : public QMultiHash<QString, Word>
{
public:
    inline WordList() {}
    void append(const Word& t) { this->insert(t.text, t); }
    void append(const WordList& l)
    { foreach(const Word& t, l) { append(t); } }
    const Word& at(int i) const
    { return values().at(i); }
};
#else /* USE_MULTI_HASH */
typedef QList<Word> WordList;
#endif /* USE_MULTI_HASH */

typedef QHash<QString /* File name */, WordList> FileWordList;

}

inline QDebug operator<<(QDebug debug, const SpellChecker::Word& word)
{
    debug.nospace() << word.text << "(" << word.lineNumber << ":" << word.columnNumber << ")";
    return debug.space();
}

#endif // WORD_H
