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

#include <QString>
#include <QMultiHash>
#include <QDebug>
#include <QStringList>

namespace SpellChecker {

/*! \brief Alias for a QSet with strings, in the same spirit as QStringList.
 *
 * It should probably not be called QStringSet (the first letter being a 'Q')
 * since it will probably get confused with something in Qt. */
using QStringSet = QSet<QString>;

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
    { for(const Word& t: l) { append(t); } }
};

typedef WordList::iterator WordListIter;
typedef WordList::const_iterator WordListConstIter;
#else /* USE_MULTI_HASH */
typedef QList<Word> WordList;
#endif /* USE_MULTI_HASH */

typedef QHash<QString /* File name */, WordList> FileWordList;

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

} // namespace SpellChecker

inline QDebug operator<<(QDebug debug, const SpellChecker::Word& word)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << word.text << "(" << word.lineNumber << ":" << word.columnNumber << ")";
    return debug;
}
