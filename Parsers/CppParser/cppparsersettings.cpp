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

#include "cppparsersettings.h"
#include "cppparserconstants.h"

#include "..\..\spellcheckerconstants.h"

using namespace SpellChecker::CppSpellChecker::Internal;
using namespace SpellChecker::CppSpellChecker;

CppParserSettings::CppParserSettings() :
    QObject(NULL)
{
    setDefaults();
}
//--------------------------------------------------

CppParserSettings::CppParserSettings(const CppParserSettings &settings) :
    QObject(NULL)
{
    removeEmailAddresses = settings.removeEmailAddresses;
    checkQtKeywords = settings.checkQtKeywords;
    checkAllCapsWords = settings.checkAllCapsWords;
    wordsWithNumberOption = settings.wordsWithNumberOption;
    wordsWithUnderscoresOption = settings.wordsWithUnderscoresOption;
    camelCaseWordOption = settings.camelCaseWordOption;
    removeWordsThatAppearInSource = settings.removeWordsThatAppearInSource;
    wordsWithDotsOption = settings.wordsWithDotsOption;
}
//--------------------------------------------------

CppParserSettings::~CppParserSettings()
{
}
//--------------------------------------------------

void CppParserSettings::loadFromSettings(QSettings *settings)
{
    setDefaults();

    settings->beginGroup(QLatin1String(Constants::CORE_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(Constants::CORE_PARSERS_GROUP));
    settings->beginGroup(QLatin1String(Parsers::CppParser::Constants::CPP_PARSER_GROUP));

    checkQtKeywords = settings->value(QLatin1String(Parsers::CppParser::Constants::CHECK_QT_KEYWORDS), checkQtKeywords).toBool();
    checkAllCapsWords = settings->value(QLatin1String(Parsers::CppParser::Constants::CHECK_CAPS), checkAllCapsWords).toBool();
    wordsWithNumberOption = static_cast<WordsWithNumbersOption>(settings->value(QLatin1String(Parsers::CppParser::Constants::CHECK_NUMBERS), wordsWithNumberOption).toInt());
    wordsWithUnderscoresOption = static_cast<WordsWithUnderscoresOption>(settings->value(QLatin1String(Parsers::CppParser::Constants::CHECK_UNDERSCORES), wordsWithUnderscoresOption).toInt());
    camelCaseWordOption = static_cast<CamelCaseWordOption>(settings->value(QLatin1String(Parsers::CppParser::Constants::CHECK_CAMELCASE), camelCaseWordOption).toInt());
    removeWordsThatAppearInSource = settings->value(QLatin1String(Parsers::CppParser::Constants::REMOVE_WORDS_SOURCE), removeWordsThatAppearInSource).toBool();
    removeEmailAddresses = settings->value(QLatin1String(Parsers::CppParser::Constants::REMOVE_EMAIL_ADDRESSES), removeEmailAddresses).toBool();
    wordsWithDotsOption = static_cast<WordsWithDotsOption>(settings->value(QLatin1String(Parsers::CppParser::Constants::CHECK_DOTS), wordsWithDotsOption).toInt());

    settings->endGroup(); /* CPP_PARSER_GROUP */
    settings->endGroup(); /* CORE_PARSERS_GROUP */
    settings->endGroup(); /* CORE_SETTINGS_GROUP */
    settings->sync();
}
//--------------------------------------------------


void CppParserSettings::saveToSetting(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(Constants::CORE_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(Constants::CORE_PARSERS_GROUP));
    settings->beginGroup(QLatin1String(Parsers::CppParser::Constants::CPP_PARSER_GROUP));

    settings->setValue(QLatin1String(Parsers::CppParser::Constants::CHECK_QT_KEYWORDS), checkQtKeywords);
    settings->setValue(QLatin1String(Parsers::CppParser::Constants::CHECK_CAPS), checkAllCapsWords);
    settings->setValue(QLatin1String(Parsers::CppParser::Constants::CHECK_NUMBERS), wordsWithNumberOption);
    settings->setValue(QLatin1String(Parsers::CppParser::Constants::CHECK_UNDERSCORES), wordsWithUnderscoresOption);
    settings->setValue(QLatin1String(Parsers::CppParser::Constants::CHECK_CAMELCASE), camelCaseWordOption);
    settings->setValue(QLatin1String(Parsers::CppParser::Constants::REMOVE_WORDS_SOURCE), removeWordsThatAppearInSource);
    settings->setValue(QLatin1String(Parsers::CppParser::Constants::REMOVE_EMAIL_ADDRESSES), removeEmailAddresses);
    settings->setValue(QLatin1String(Parsers::CppParser::Constants::CHECK_DOTS), wordsWithDotsOption);

    settings->endGroup(); /* CPP_PARSER_GROUP */
    settings->endGroup(); /* CORE_PARSERS_GROUP */
    settings->endGroup(); /* CORE_SETTINGS_GROUP */
    settings->sync();
}
//--------------------------------------------------

void CppParserSettings::setDefaults()
{
    removeEmailAddresses = true;
    checkQtKeywords = false;
    checkAllCapsWords = false;
    wordsWithNumberOption = SplitWordsOnNumbers;
    wordsWithUnderscoresOption = SplitWordsOnUnderscores;
    camelCaseWordOption = SplitWordsOnCamelCase;
    removeWordsThatAppearInSource = true;
    wordsWithDotsOption = SplitWordsOnDots;
}
//--------------------------------------------------

CppParserSettings &CppParserSettings::operator=(const CppParserSettings &other)
{
    bool settingsSame = (operator==(other));
    if(settingsSame == false) {
        this->checkQtKeywords = other.checkQtKeywords;
        this->checkAllCapsWords = other.checkAllCapsWords;
        this->wordsWithNumberOption = other.wordsWithNumberOption;
        this->wordsWithUnderscoresOption = other.wordsWithUnderscoresOption;
        this->camelCaseWordOption = other.camelCaseWordOption;
        this->removeWordsThatAppearInSource = other.removeWordsThatAppearInSource;
        this->removeEmailAddresses = other.removeEmailAddresses;
        this->wordsWithDotsOption = other.wordsWithDotsOption;
        emit settingsChanged();
    }

    return *this;
}
//--------------------------------------------------

bool CppParserSettings::operator==(const CppParserSettings &other) const
{
    bool different = false;
    different = different | (checkQtKeywords != other.checkQtKeywords);
    different = different | (checkAllCapsWords != other.checkAllCapsWords);
    different = different | (wordsWithNumberOption != other.wordsWithNumberOption);
    different = different | (wordsWithUnderscoresOption != other.wordsWithUnderscoresOption);
    different = different | (camelCaseWordOption != other.camelCaseWordOption);
    different = different | (removeWordsThatAppearInSource != other.removeWordsThatAppearInSource);
    different = different | (removeEmailAddresses != other.removeEmailAddresses);
    different = different | (wordsWithDotsOption != other.wordsWithDotsOption);
    return (different == false);
}
//--------------------------------------------------

