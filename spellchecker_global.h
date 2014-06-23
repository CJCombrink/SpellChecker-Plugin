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

#ifndef SPELLCHECKER_GLOBAL_H
#define SPELLCHECKER_GLOBAL_H

#include <QtGlobal>

#if defined(SPELLCHECKER_LIBRARY)
#  define SPELLCHECKERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define SPELLCHECKERSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // SPELLCHECKER_GLOBAL_H

