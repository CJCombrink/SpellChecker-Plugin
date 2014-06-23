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

#include "idocumentparser.h"

using namespace SpellChecker;

IDocumentParser::IDocumentParser(QObject *parent) :
    QObject(parent)
{
}
//--------------------------------------------------

IDocumentParser::~IDocumentParser()
{
}
//--------------------------------------------------

FileWordList IDocumentParser::parseFiles(const QStringList &fileNames)
{
    FileWordList fileWordList;
    WordList wordList;
    foreach(const QString& file, fileNames) {
        wordList = parseFile(file);
        fileWordList.insert(file, wordList);
    }
    return fileWordList;
}
//--------------------------------------------------

void IDocumentParser::getWordsFromSplitString(const QStringList &stringList, const Word &word, WordList &wordList)
{
    /* Now that the words are split, they need to be added to the WordList correctly */
    int numbSplitWords = stringList.count();
    int currentPos = 0;
    for(int wordIdx = 0; wordIdx < numbSplitWords; ++wordIdx) {
        Word newWord;
        newWord.text = stringList.at(wordIdx);
        newWord.fileName = word.fileName;
        currentPos = (word.text).indexOf(newWord.text, currentPos);
        newWord.columnNumber = word.columnNumber + currentPos;
        newWord.lineNumber = word.lineNumber;
        newWord.length = newWord.text.length();
        newWord.start = word.start + currentPos;
        newWord.end = newWord.start + newWord.length;
        currentPos = currentPos + newWord.length;
        /* Add the word to the end of the word list so that it can be checked against the settings later on */
        wordList.append(newWord);
    }
}
//--------------------------------------------------
