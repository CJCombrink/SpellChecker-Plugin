/**************************************************************************
**
** Copyright (c) 2026 Marcel Petrick
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

namespace SpellChecker {
namespace Parsers {
namespace QmlParser {
namespace Constants {

const char QML_PARSER_GROUP[]       = "QmlParser";             /*!< Settings group for QML parser options. */
const char WHAT_TO_CHECK[]          = "WhatToCheck";           /*!< Settings key for checked token types. */
const char REMOVE_EMAIL_ADDRESSES[] = "removeEmailAddresses";  /*!< Settings key for email filtering. */
const char CHECK_CAPS[]             = "checkAllCapsWords";     /*!< Settings key for all-caps word handling. */
const char REMOVE_WEBSITES[]        = "removeWebsites";        /*!< Settings key for website filtering. */

} // namespace Constants
} // namespace QmlParser
} // namespace Parsers
} // namespace SpellChecker
