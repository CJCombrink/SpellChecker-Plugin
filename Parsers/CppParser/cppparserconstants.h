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

#ifndef CPP_PARSER_CONSTANTS_H
#define CPP_PARSER_CONSTANTS_H

namespace SpellChecker {
namespace Parsers {
namespace CppParser {
namespace Constants {

/* Pattern based on pattern found on http://gskinner.com/RegExr/ for email addresses */
const char EMAIL_ADDRESS_REGEXP_PATTERN[]   = "[\\w-\\.]+@((?:[\\w]+\\.)+)[a-zA-Z]{2,4}";
/* Custom pattern used for website addresses. Probably not optimal nor a good option. */
const char WEBSITE_ADDRESS_REGEXP_PATTERN[] = "((http(s)?):\\/\\/)?(\\w+\\.){1,10}[0-9A-z./?=#%\\-]+";
/* Pattern for characters that can be part of a website URL. */
const char WEBSITE_CHARS_REGEXP_PATTERN[]   = "[/:\\?=#%-]";
/* Pattern for CPP files. */
const char CPP_SOURCE_FILES_REGEXP_PATTERN[]= ".*\\.(cpp|h)$";

const char CPP_PARSER_GROUP[]       = "CppParser";
const char WHAT_TO_CHECK[]          = "WhatToCheck";
const char CHECK_QT_KEYWORDS[]      = "CheckQtKeywords";
const char CHECK_CAPS[]             = "checkAllCapsWords";
const char CHECK_NUMBERS[]          = "wordsWithNumberOption";
const char CHECK_UNDERSCORES[]      = "wordsWithUnderscoresOption";
const char CHECK_CAMELCASE[]        = "camelCaseWordOption";
const char REMOVE_WORDS_SOURCE[]    = "removeWordsThatAppearInSource";
const char REMOVE_EMAIL_ADDRESSES[] = "removeEmailAddresses";
const char CHECK_DOTS[]             = "wordsWithDotsOption";
const char REMOVE_WEBSITES[]        = "removeWebsites";

} // namespace Constants
} // namespace CppParser
} // namespace Parsers
} // namespace SpellChecker

#endif // CPP_PARSER_CONSTANTS_H
