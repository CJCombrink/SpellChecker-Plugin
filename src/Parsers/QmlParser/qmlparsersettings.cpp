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

#include "qmlparsersettings.h"

#include "../../spellcheckerconstants.h"
#include "qmlparserconstants.h"

using namespace SpellChecker::QmlSpellChecker::Internal;

QmlParserSettings::QmlParserSettings()
  : QObject( nullptr )
{
  setDefaults();
}

QmlParserSettings::QmlParserSettings( const QmlParserSettings& settings )
  : QObject( nullptr )
{
  whatToCheck          = settings.whatToCheck;
  removeEmailAddresses = settings.removeEmailAddresses;
  checkAllCapsWords    = settings.checkAllCapsWords;
  removeWebsites       = settings.removeWebsites;
}

QmlParserSettings::~QmlParserSettings() = default;

void QmlParserSettings::loadFromSettings( Utils::QtcSettings* settings )
{
  setDefaults();

  settings->beginGroup( SpellChecker::Constants::CORE_SETTINGS_GROUP );
  settings->beginGroup( SpellChecker::Constants::CORE_PARSERS_GROUP );
  settings->beginGroup( SpellChecker::Parsers::QmlParser::Constants::QML_PARSER_GROUP );

  whatToCheck          = static_cast<WhatToCheckOptions>( settings->value( SpellChecker::Parsers::QmlParser::Constants::WHAT_TO_CHECK, int( whatToCheck ) ).toInt() );
  removeEmailAddresses = settings->value( SpellChecker::Parsers::QmlParser::Constants::REMOVE_EMAIL_ADDRESSES, removeEmailAddresses ).toBool();
  checkAllCapsWords    = settings->value( SpellChecker::Parsers::QmlParser::Constants::CHECK_CAPS, checkAllCapsWords ).toBool();
  removeWebsites       = settings->value( SpellChecker::Parsers::QmlParser::Constants::REMOVE_WEBSITES, removeWebsites ).toBool();

  settings->endGroup();
  settings->endGroup();
  settings->endGroup();
  settings->sync();
}

void QmlParserSettings::saveToSettings( Utils::QtcSettings* settings ) const
{
  settings->beginGroup( SpellChecker::Constants::CORE_SETTINGS_GROUP );
  settings->beginGroup( SpellChecker::Constants::CORE_PARSERS_GROUP );
  settings->beginGroup( SpellChecker::Parsers::QmlParser::Constants::QML_PARSER_GROUP );

  settings->setValue( SpellChecker::Parsers::QmlParser::Constants::WHAT_TO_CHECK, int( whatToCheck ) );
  settings->setValue( SpellChecker::Parsers::QmlParser::Constants::REMOVE_EMAIL_ADDRESSES, removeEmailAddresses );
  settings->setValue( SpellChecker::Parsers::QmlParser::Constants::CHECK_CAPS, checkAllCapsWords );
  settings->setValue( SpellChecker::Parsers::QmlParser::Constants::REMOVE_WEBSITES, removeWebsites );

  settings->endGroup();
  settings->endGroup();
  settings->endGroup();
  settings->sync();
}

void QmlParserSettings::setDefaults()
{
  whatToCheck          = CheckBoth;
  removeEmailAddresses = true;
  checkAllCapsWords    = false;
  removeWebsites       = true;
}

QmlParserSettings& QmlParserSettings::operator=( const QmlParserSettings& other )
{
  const bool settingsSame = ( operator==( other ) );
  if( settingsSame == false ) {
    whatToCheck          = other.whatToCheck;
    removeEmailAddresses = other.removeEmailAddresses;
    checkAllCapsWords    = other.checkAllCapsWords;
    removeWebsites       = other.removeWebsites;
    emit settingsChanged();
  }

  return *this;
}

bool QmlParserSettings::operator==( const QmlParserSettings& other ) const
{
  return ( whatToCheck == other.whatToCheck )
         && ( removeEmailAddresses == other.removeEmailAddresses )
         && ( checkAllCapsWords == other.checkAllCapsWords )
         && ( removeWebsites == other.removeWebsites );
}
