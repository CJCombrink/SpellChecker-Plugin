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

#include <cppeditor/cppeditor.h>
#include "spellcheckquickfix.h"
#include "spellcheckercore.h"

using namespace SpellChecker;

namespace SpellChecker {
namespace Internal {

class SpellCheckReplaceWordOperation : public TextEditor::QuickFixOperation
{
public:
    SpellCheckReplaceWordOperation(const WordList &words, const QString &replacement):
        m_words(words),
        m_replacement(replacement)
    {
    }

    QString description() const Q_DECL_OVERRIDE
    {
        return QString(QLatin1String("Replace with '%1'")).arg(m_replacement);
    }

    void perform() Q_DECL_OVERRIDE
    {
        SpellCheckerCore* core = SpellCheckerCore::instance();
        if (core) {
            core->replaceWordsInCurrentEditor(m_words, m_replacement);
        }
    }

private:
    WordList m_words;
    QString m_replacement;
};
//--------------------------------------------------

class SpellCheckIgnoreWordOperation : public TextEditor::QuickFixOperation
{
public:
    QString description() const Q_DECL_OVERRIDE
    {
        return QLatin1String("Ignore word");
    }

    void perform() Q_DECL_OVERRIDE
    {
        SpellCheckerCore* core = SpellCheckerCore::instance();
        if (core) {
            core->ignoreWordUnderCursor();
        }
    }
};
//--------------------------------------------------

class SpellCheckAddWordOperation : public TextEditor::QuickFixOperation
{
public:
    QString description() const Q_DECL_OVERRIDE
    {
        return QLatin1String("Add word to dictionary");
    }

    void perform() Q_DECL_OVERRIDE
    {
        SpellCheckerCore* core = SpellCheckerCore::instance();
        if (core) {
            core->addWordUnderCursor();
        }
    }
};

}
}

//--------------------------------------------------

void SpellCheckCppQuickFixFactory::match(const CppEditor::Internal::CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result)
{
    Q_UNUSED(interface)

    SpellCheckerCore* core = SpellCheckerCore::instance();
    if (!core)
        return;

    Word word;
    if (core->isWordUnderCursorMistake(word)) {
        WordList words;
        words.append(word);

        int priority = word.suggestions.count();
        result.reserve(word.suggestions.count() + 2);
        foreach (const QString &suggestion, word.suggestions) {
            TextEditor::QuickFixOperation *quickFix = new Internal::SpellCheckReplaceWordOperation(words, suggestion);
            quickFix->setPriority(priority--);
            result.append(TextEditor::QuickFixOperation::Ptr(quickFix));
        }
        result.prepend(TextEditor::QuickFixOperation::Ptr(new Internal::SpellCheckIgnoreWordOperation()));
        result.prepend(TextEditor::QuickFixOperation::Ptr(new Internal::SpellCheckAddWordOperation()));
    }
}
//--------------------------------------------------
