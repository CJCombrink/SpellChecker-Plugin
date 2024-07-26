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
#include <QtGlobal>

namespace SpellChecker {
namespace Constants {

const char SPELLCHECK_MISTAKE_ID[] = "SpellChecker.Spelling.Mistake";

const char MENU_ID[]           = "SpellChecker.Menu";
const char CONTEXT_MENU_ID[]   = "SpellChecker.ContextMenu";
const char ACTION_SUGGEST_ID[] = "SpellChecker.ActionSuggest";
const char ACTION_IGNORE_ID[]  = "SpellChecker.ActionIgnore";
const char ACTION_ADD_ID[]     = "SpellChecker.ActionAdd";
const char ACTION_LUCKY_ID[]   = "SpellChecker.ActionLucky";
const char ACTION_HOLDER1_ID[] = "SpellChecker.ActionHolder1";
const char ACTION_HOLDER2_ID[] = "SpellChecker.ActionHolder2";
const char ACTION_HOLDER3_ID[] = "SpellChecker.ActionHolder3";
const char ACTION_HOLDER4_ID[] = "SpellChecker.ActionHolder4";
const char ACTION_HOLDER5_ID[] = "SpellChecker.ActionHolder5";

const char CORE_SETTINGS_GROUP[]      = "SpellCheckerPlugin";
const char CORE_SPELLCHECKERS_GROUP[] = "SpellCheckers";
const char CORE_PARSERS_GROUP[]       = "Parsers";
const char CORE_SETTINGS_OP_GROUP[]   = "OutputPane";

const char SETTING_ACTIVE_SPELLCHECKER[]      = "ActiveSpellChecker";
const char SETTING_ONLY_PARSE_CURRENT[]       = "OnlyParseCurrentFile";
const char SETTING_CHECK_EXTERNAL[]           = "CheckExternal";
const char PROJECTS_TO_IGNORE[]               = "ProjectsToIgnore";
const char REPLACE_ALL_FROM_RIGHT_CLICK[]     = "ReplaceAllFromRightClick";
const char SETTINGS_OUTPUT_PANE_COL_WORD[]    = "ColWord";
const char SETTINGS_OUTPUT_PANE_COL_LITERAL[] = "ColLiteral";
const char SETTINGS_OUTPUT_PANE_COL_LINE[]    = "ColLine";
const char SETTINGS_OUTPUT_PANE_COL_COLUMN[]  = "ColColumn";
const char SETTINGS_ERRORS_COLOR[]            = "ErrorsColor";

const char OUTPUT_PANE_TITLE[] = QT_TRANSLATE_NOOP( "SpellChecker::Internal::OutputPane", "Spelling Mistakes" );

enum MistakesModelColumn {
  MISTAKE_COLUMN_IDX = 0,
  MISTAKE_COLUMN_WORD,
  MISTAKE_COLUMN_SUGGESTIONS,
  MISTAKE_COLUMN_LITERAL,
  MISTAKE_COLUMN_LINE,
  MISTAKE_COLUMN_COLUMN,
  MISTAKE_COLUMN_COUNT
};

/* Perhaps this is not the best way to do it. This provides for index based lookup
 * using the MistakesModelColumn enum into the char array */
const char MISTAKES_MODEL_COLUMN_NAMES[][20] = { QT_TRANSLATE_NOOP( "SpellChecker::Internal::SpellingMistakesModel", "#" ),
                                                 QT_TRANSLATE_NOOP( "SpellChecker::Internal::SpellingMistakesModel", "Word" ),
                                                 QT_TRANSLATE_NOOP( "SpellChecker::Internal::SpellingMistakesModel", "Suggestions" ),
                                                 QT_TRANSLATE_NOOP( "SpellChecker::Internal::SpellingMistakesModel", "Literal" ),
                                                 QT_TRANSLATE_NOOP( "SpellChecker::Internal::SpellingMistakesModel", "Line" ),
                                                 QT_TRANSLATE_NOOP( "SpellChecker::Internal::SpellingMistakesModel", "Col" ) };

/* Image resource paths */
const char ICON_SPELLCHECKERPLUGIN_OPTIONS[] = ":/spellcheckerplugin/images/optionspageicon.png";
const char ICON_OUTPUTPANE_LUCKY_BUTTON[]    = ":/spellcheckerplugin/images/clover.png";

} // namespace SpellChecker
} // namespace Constants
