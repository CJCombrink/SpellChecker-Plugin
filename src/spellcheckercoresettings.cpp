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

#include "spellcheckerconstants.h"
#include "spellcheckercoresettings.h"

using namespace SpellChecker::Internal;

SpellCheckerCoreSettings::SpellCheckerCoreSettings()
  : QObject( nullptr )
  , activeSpellChecker()
  , onlyParseCurrentFile( true )
  , checkExternalFiles( false )
  , projectsToIgnore()
  , replaceAllFromRightClick( true )
{}
// --------------------------------------------------

SpellCheckerCoreSettings::SpellCheckerCoreSettings( const SpellCheckerCoreSettings& settings )
  : QObject( settings.parent() )
  , activeSpellChecker( settings.activeSpellChecker )
  , onlyParseCurrentFile( settings.onlyParseCurrentFile )
  , checkExternalFiles( settings.checkExternalFiles )
  , projectsToIgnore( settings.projectsToIgnore )
  , replaceAllFromRightClick( settings.replaceAllFromRightClick )
{}
// --------------------------------------------------

SpellCheckerCoreSettings::~SpellCheckerCoreSettings()
{}
// --------------------------------------------------

void SpellCheckerCoreSettings::saveToSettings( Utils::QtcSettings* settings ) const
{
  settings->beginGroup( Constants::CORE_SETTINGS_GROUP );
  settings->setValue( Constants::SETTING_ACTIVE_SPELLCHECKER,  activeSpellChecker );
  settings->setValue( Constants::SETTING_ONLY_PARSE_CURRENT,   onlyParseCurrentFile );
  settings->setValue( Constants::SETTING_CHECK_EXTERNAL,       checkExternalFiles );
  settings->setValue( Constants::PROJECTS_TO_IGNORE,           projectsToIgnore );
  settings->setValue( Constants::REPLACE_ALL_FROM_RIGHT_CLICK, replaceAllFromRightClick );
  settings->endGroup(); /* CORE_SETTINGS_GROUP */
  settings->sync();
}
// --------------------------------------------------

void SpellCheckerCoreSettings::loadFromSettings( Utils::QtcSettings* settings )
{
  settings->beginGroup( Constants::CORE_SETTINGS_GROUP );
  activeSpellChecker       = settings->value( Constants::SETTING_ACTIVE_SPELLCHECKER, activeSpellChecker ).toString();
  onlyParseCurrentFile     = settings->value( Constants::SETTING_ONLY_PARSE_CURRENT, onlyParseCurrentFile ).toBool();
  checkExternalFiles       = settings->value( Constants::SETTING_CHECK_EXTERNAL, checkExternalFiles ).toBool();
  projectsToIgnore         = settings->value( Constants::PROJECTS_TO_IGNORE, projectsToIgnore ).toStringList();
  replaceAllFromRightClick = settings->value( Constants::REPLACE_ALL_FROM_RIGHT_CLICK, replaceAllFromRightClick ).toBool();
  settings->endGroup(); /* CORE_SETTINGS_GROUP */
}
// --------------------------------------------------

SpellCheckerCoreSettings& SpellCheckerCoreSettings::operator=( const SpellCheckerCoreSettings& other )
{
  bool settingsSame = ( operator==( other ) );
  if( settingsSame == false ) {
    this->activeSpellChecker       = other.activeSpellChecker;
    this->onlyParseCurrentFile     = other.onlyParseCurrentFile;
    this->checkExternalFiles       = other.checkExternalFiles;
    this->projectsToIgnore         = other.projectsToIgnore;
    this->replaceAllFromRightClick = other.replaceAllFromRightClick;
    emit settingsChanged();
  }
  return *this;
}
// --------------------------------------------------

bool SpellCheckerCoreSettings::operator==( const SpellCheckerCoreSettings& other ) const
{
  bool different = false;
  different = different | ( activeSpellChecker != other.activeSpellChecker );
  different = different | ( onlyParseCurrentFile != other.onlyParseCurrentFile );
  different = different | ( checkExternalFiles != other.checkExternalFiles );
  different = different | ( projectsToIgnore != other.projectsToIgnore );
  different = different | ( replaceAllFromRightClick != other.replaceAllFromRightClick );
  return ( different == false );
}
// --------------------------------------------------
