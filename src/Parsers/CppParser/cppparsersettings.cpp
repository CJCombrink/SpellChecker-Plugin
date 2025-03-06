/***************
 ***********************************************************
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

#include "cppparserconstants.h"
#include "cppparsersettings.h"

#include "../../spellcheckerconstants.h"

using namespace SpellChecker::CppSpellChecker::Internal;
using namespace SpellChecker::CppSpellChecker;

CppParserSettings::CppParserSettings()
  : QObject( nullptr )
{
  setDefaults();
}
// --------------------------------------------------

CppParserSettings::CppParserSettings( const CppParserSettings& settings )
  : QObject( nullptr )
{
  whatToCheck                   = settings.whatToCheck;
  commentsToCheck               = settings.commentsToCheck;
  removeEmailAddresses          = settings.removeEmailAddresses;
  checkQtKeywords               = settings.checkQtKeywords;
  checkAllCapsWords             = settings.checkAllCapsWords;
  wordsWithNumberOption         = settings.wordsWithNumberOption;
  wordsWithUnderscoresOption    = settings.wordsWithUnderscoresOption;
  camelCaseWordOption           = settings.camelCaseWordOption;
  removeWordsThatAppearInSource = settings.removeWordsThatAppearInSource;
  wordsWithDotsOption           = settings.wordsWithDotsOption;
  removeWebsites                = settings.removeWebsites;
  removeFirstComment            = settings.removeFirstComment;
}
// --------------------------------------------------

CppParserSettings::~CppParserSettings()
{}
// --------------------------------------------------

void CppParserSettings::loadFromSettings( Utils::QtcSettings* settings )
{
  setDefaults();

  settings->beginGroup( Constants::CORE_SETTINGS_GROUP );
  settings->beginGroup( Constants::CORE_PARSERS_GROUP );
  settings->beginGroup( Parsers::CppParser::Constants::CPP_PARSER_GROUP );

  whatToCheck                   = static_cast<WhatToCheckOptions>( settings->value( Parsers::CppParser::Constants::WHAT_TO_CHECK, int(whatToCheck) ).toInt() );
  commentsToCheck               = static_cast<CommentsToCheckOptions>( settings->value( Parsers::CppParser::Constants::COMMENTS_TO_CHECK, int(commentsToCheck) ).toInt() );
  checkQtKeywords               = settings->value( Parsers::CppParser::Constants::CHECK_QT_KEYWORDS, checkQtKeywords ).toBool();
  checkAllCapsWords             = settings->value( Parsers::CppParser::Constants::CHECK_CAPS, checkAllCapsWords ).toBool();
  wordsWithNumberOption         = static_cast<WordsWithNumbersOption>( settings->value( Parsers::CppParser::Constants::CHECK_NUMBERS, wordsWithNumberOption ).toInt() );
  wordsWithUnderscoresOption    = static_cast<WordsWithUnderscoresOption>( settings->value( Parsers::CppParser::Constants::CHECK_UNDERSCORES, wordsWithUnderscoresOption ).toInt() );
  camelCaseWordOption           = static_cast<CamelCaseWordOption>( settings->value( Parsers::CppParser::Constants::CHECK_CAMELCASE, camelCaseWordOption ).toInt() );
  removeWordsThatAppearInSource = settings->value( Parsers::CppParser::Constants::REMOVE_WORDS_SOURCE, removeWordsThatAppearInSource ).toBool();
  removeEmailAddresses          = settings->value( Parsers::CppParser::Constants::REMOVE_EMAIL_ADDRESSES, removeEmailAddresses ).toBool();
  wordsWithDotsOption           = static_cast<WordsWithDotsOption>( settings->value( Parsers::CppParser::Constants::CHECK_DOTS, wordsWithDotsOption ).toInt() );
  removeWebsites                = settings->value( Parsers::CppParser::Constants::REMOVE_WEBSITES, removeWebsites ).toBool();
  removeFirstComment            = settings->value( Parsers::CppParser::Constants::REMOVE_FIRST_COMMENT, removeFirstComment ).toBool();

  settings->endGroup(); /* CPP_PARSER_GROUP */
  settings->endGroup(); /* CORE_PARSERS_GROUP */
  settings->endGroup(); /* CORE_SETTINGS_GROUP */
  settings->sync();
}
// --------------------------------------------------


void CppParserSettings::saveToSetting(Utils::QtcSettings *settings ) const
{
  settings->beginGroup( Constants::CORE_SETTINGS_GROUP );
  settings->beginGroup( Constants::CORE_PARSERS_GROUP );
  settings->beginGroup( Parsers::CppParser::Constants::CPP_PARSER_GROUP );

  settings->setValue( Parsers::CppParser::Constants::WHAT_TO_CHECK,          int(whatToCheck) );
  settings->setValue( Parsers::CppParser::Constants::COMMENTS_TO_CHECK,      int(commentsToCheck) );
  settings->setValue( Parsers::CppParser::Constants::CHECK_QT_KEYWORDS,      checkQtKeywords );
  settings->setValue( Parsers::CppParser::Constants::CHECK_CAPS,             checkAllCapsWords );
  settings->setValue( Parsers::CppParser::Constants::CHECK_NUMBERS,          wordsWithNumberOption );
  settings->setValue( Parsers::CppParser::Constants::CHECK_UNDERSCORES,      wordsWithUnderscoresOption );
  settings->setValue( Parsers::CppParser::Constants::CHECK_CAMELCASE,        camelCaseWordOption );
  settings->setValue( Parsers::CppParser::Constants::REMOVE_WORDS_SOURCE,    removeWordsThatAppearInSource );
  settings->setValue( Parsers::CppParser::Constants::REMOVE_EMAIL_ADDRESSES, removeEmailAddresses );
  settings->setValue( Parsers::CppParser::Constants::CHECK_DOTS,             wordsWithDotsOption );
  settings->setValue( Parsers::CppParser::Constants::REMOVE_WEBSITES,        removeWebsites );
  settings->setValue( Parsers::CppParser::Constants::REMOVE_FIRST_COMMENT,   removeFirstComment );

  settings->endGroup(); /* CPP_PARSER_GROUP */
  settings->endGroup(); /* CORE_PARSERS_GROUP */
  settings->endGroup(); /* CORE_SETTINGS_GROUP */
  settings->sync();
}
// --------------------------------------------------

void CppParserSettings::setDefaults()
{
  whatToCheck                   = { CheckComments | CheckStringLiterals };
  commentsToCheck               = CommentsBoth;
  removeEmailAddresses          = true;
  checkQtKeywords               = false;
  checkAllCapsWords             = false;
  wordsWithNumberOption         = SplitWordsOnNumbers;
  wordsWithUnderscoresOption    = SplitWordsOnUnderscores;
  camelCaseWordOption           = SplitWordsOnCamelCase;
  removeWordsThatAppearInSource = true;
  wordsWithDotsOption           = SplitWordsOnDots;
  removeWebsites                = false;
  removeFirstComment            = false;
}
// --------------------------------------------------

CppParserSettings& CppParserSettings::operator=( const CppParserSettings& other )
{
  bool settingsSame = ( operator==( other ) );
  if( settingsSame == false ) {
    this->whatToCheck                   = other.whatToCheck;
    this->commentsToCheck               = other.commentsToCheck;
    this->checkQtKeywords               = other.checkQtKeywords;
    this->checkAllCapsWords             = other.checkAllCapsWords;
    this->wordsWithNumberOption         = other.wordsWithNumberOption;
    this->wordsWithUnderscoresOption    = other.wordsWithUnderscoresOption;
    this->camelCaseWordOption           = other.camelCaseWordOption;
    this->removeWordsThatAppearInSource = other.removeWordsThatAppearInSource;
    this->removeEmailAddresses          = other.removeEmailAddresses;
    this->wordsWithDotsOption           = other.wordsWithDotsOption;
    this->removeWebsites                = other.removeWebsites;
    this->removeFirstComment            = other.removeFirstComment;
    emit settingsChanged();
  }

  return *this;
}
// --------------------------------------------------

bool CppParserSettings::operator==( const CppParserSettings& other ) const
{
  bool different = false;
  different = different | ( whatToCheck != other.whatToCheck );
  different = different | ( commentsToCheck != other.commentsToCheck );
  different = different | ( checkQtKeywords != other.checkQtKeywords );
  different = different | ( checkAllCapsWords != other.checkAllCapsWords );
  different = different | ( wordsWithNumberOption != other.wordsWithNumberOption );
  different = different | ( wordsWithUnderscoresOption != other.wordsWithUnderscoresOption );
  different = different | ( camelCaseWordOption != other.camelCaseWordOption );
  different = different | ( removeWordsThatAppearInSource != other.removeWordsThatAppearInSource );
  different = different | ( removeEmailAddresses != other.removeEmailAddresses );
  different = different | ( wordsWithDotsOption != other.wordsWithDotsOption );
  different = different | ( removeWebsites != other.removeWebsites );
  different = different | ( removeFirstComment != other.removeFirstComment );
  return ( different == false );
}
// --------------------------------------------------
