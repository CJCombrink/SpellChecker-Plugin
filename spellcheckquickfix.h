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

#ifndef SPELLCHECKER_SPELLCHECKQUICKFIX_H
#define SPELLCHECKER_SPELLCHECKQUICKFIX_H

#include <cppeditor/cppquickfix.h>

namespace SpellChecker {

/*! \brief QuickFixFactory implementation for fixing spelling mistakes inside C++ editor.
 *
 * It would be more generic to implement TextEditor::QuickFixFactory, but each editor actually only
 * uses a single quick fix factory at the moment (QtCreator 3.2); thus for C++ a CppQuickFixFactory
 * must be created instead. */
class SpellCheckCppQuickFixFactory: public CppEditor::CppQuickFixFactory
{
public:
    void match(const CppEditor::Internal::CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) Q_DECL_OVERRIDE;
};

} // namespace SpellChecker

#endif // SPELLCHECKER_SPELLCHECKQUICKFIX_H
