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

#include "spellcheckercoresettings.h"
#include "spellcheckerconstants.h"

using namespace SpellChecker::Internal;

SpellCheckerCoreSettings::SpellCheckerCoreSettings() :
    QObject(NULL),
    activeSpellChecker(),
    onlyParseCurrentFile(true)
{
}
//--------------------------------------------------

SpellCheckerCoreSettings::SpellCheckerCoreSettings(const SpellCheckerCoreSettings &settings) :
    QObject(settings.parent()),
    activeSpellChecker(settings.activeSpellChecker),
    onlyParseCurrentFile(settings.onlyParseCurrentFile)
{
}
//--------------------------------------------------

SpellCheckerCoreSettings::~SpellCheckerCoreSettings()
{
}
//--------------------------------------------------

void SpellCheckerCoreSettings::saveToSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(Constants::CORE_SETTINGS_GROUP));
    settings->setValue(QLatin1String(Constants::SETTING_ACTIVE_SPELLCHECKER), activeSpellChecker);
    settings->setValue(QLatin1String(Constants::SETTING_ONLY_PARSE_CURRENT), onlyParseCurrentFile);
    settings->endGroup(); /* CORE_SETTINGS_GROUP */
    settings->sync();
}
//--------------------------------------------------

void SpellCheckerCoreSettings::loadFromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(Constants::CORE_SETTINGS_GROUP));
    activeSpellChecker   = settings->value(QLatin1String(Constants::SETTING_ACTIVE_SPELLCHECKER), activeSpellChecker).toString();
    onlyParseCurrentFile = settings->value(QLatin1String(Constants::SETTING_ONLY_PARSE_CURRENT), onlyParseCurrentFile).toBool();
    settings->endGroup(); /* CORE_SETTINGS_GROUP */
}
//--------------------------------------------------

SpellCheckerCoreSettings &SpellCheckerCoreSettings::operator =(const SpellCheckerCoreSettings &other)
{
    bool settingsSame = (operator==(other));
    if(settingsSame == false) {
        this->activeSpellChecker   = other.activeSpellChecker;
        this->onlyParseCurrentFile = other.onlyParseCurrentFile;
        emit settingsChanged();
    }
    return *this;
}
//--------------------------------------------------

bool SpellCheckerCoreSettings::operator ==(const SpellCheckerCoreSettings &other) const
{
    bool different = false;
    different = different | (activeSpellChecker   != other.activeSpellChecker);
    different = different | (onlyParseCurrentFile != other.onlyParseCurrentFile);
    return (different == false);
}
//--------------------------------------------------

