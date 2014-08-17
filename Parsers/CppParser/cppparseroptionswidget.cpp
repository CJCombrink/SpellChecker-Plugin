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

#include "cppparseroptionswidget.h"
#include "ui_cppparseroptionswidget.h"
#include "cppparsersettings.h"
#include "cppparserconstants.h"

namespace SpellChecker {
namespace CppSpellChecker {
namespace Internal {

#define ENUM_VAL_PROPERTY "enum.Value"

CppParserOptionsWidget::CppParserOptionsWidget(const CppParserSettings* const settings, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CppParserOptionsWidget)
{
    ui->setupUi(this);
    ui->labelDescriptionEmail->setText(ui->labelDescriptionEmail->text().arg(QLatin1String(SpellChecker::Parsers::CppParser::Constants::EMAIL_ADDRESS_REGEXP_PATTERN)));
    ui->labelDescriptionWebsites->setText(ui->labelDescriptionWebsites->text().arg(QLatin1String(SpellChecker::Parsers::CppParser::Constants::WEBSITE_ADDRESS_REGEXP_PATTERN)));
    /* Set up the options for words with numbers. */
    ui->radioButtonNumbersRemove->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::RemoveWordsWithNumbers);
    ui->radioButtonNumbersSplit->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::SplitWordsOnNumbers);
    ui->radioButtonNumbersLeave->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::LeaveWordsWithNumbers);
    connect(ui->radioButtonNumbersRemove, SIGNAL(toggled(bool)), this, SLOT(radioButtonNumbersToggled()));
    connect(ui->radioButtonNumbersSplit, SIGNAL(toggled(bool)), this, SLOT(radioButtonNumbersToggled()));
    connect(ui->radioButtonNumbersLeave, SIGNAL(toggled(bool)), this, SLOT(radioButtonNumbersToggled()));
    /* Set up the options for word with underscores. */
    ui->radioButtonUnderscoresRemove->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::RemoveWordsWithUnderscores);
    ui->radioButtonUnderscoresSplit->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::SplitWordsOnUnderscores);
    ui->radioButtonUnderscoresLeave->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::LeaveWordsWithNumbers);
    connect(ui->radioButtonUnderscoresRemove, SIGNAL(toggled(bool)), this, SLOT(radioButtonUnderscoresToggled()));
    connect(ui->radioButtonUnderscoresSplit, SIGNAL(toggled(bool)), this, SLOT(radioButtonUnderscoresToggled()));
    connect(ui->radioButtonUnderscoresLeave, SIGNAL(toggled(bool)), this, SLOT(radioButtonUnderscoresToggled()));
    /* Set up the options for words in camelCase. */
    ui->radioButtonCamelRemove->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::RemoveWordsInCamelCase);
    ui->radioButtonCamelSplit->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::SplitWordsOnCamelCase);
    ui->radioButtonCamelLeave->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::LeaveWordsInCamelCase);
    connect(ui->radioButtonCamelRemove, SIGNAL(toggled(bool)), this, SLOT(radioButtonCamelCaseToggled()));
    connect(ui->radioButtonCamelSplit, SIGNAL(toggled(bool)), this, SLOT(radioButtonCamelCaseToggled()));
    connect(ui->radioButtonCamelLeave, SIGNAL(toggled(bool)), this, SLOT(radioButtonCamelCaseToggled()));
    /* Set up the options for words with dots. */
    ui->radioButtonDotsRemove->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::RemoveWordsWithDots);
    ui->radioButtonDotsSplit->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::SplitWordsOnDots);
    ui->radioButtonDotsLeave->setProperty(ENUM_VAL_PROPERTY, CppParserSettings::LeaveWordsWithDots);
    connect(ui->radioButtonDotsRemove, SIGNAL(toggled(bool)), this, SLOT(radioButtonDotsToggled()));
    connect(ui->radioButtonDotsSplit, SIGNAL(toggled(bool)), this, SLOT(radioButtonDotsToggled()));
    connect(ui->radioButtonDotsLeave, SIGNAL(toggled(bool)), this, SLOT(radioButtonDotsToggled()));

    updateWithSettings(settings);
}
//--------------------------------------------------

CppParserOptionsWidget::~CppParserOptionsWidget()
{
    delete ui;
}
//--------------------------------------------------

const CppParserSettings &CppParserOptionsWidget::settings()
{
    m_settings.removeEmailAddresses = ui->checkBoxRemoveEmailAddresses->isChecked();
    m_settings.checkQtKeywords = !ui->checkBoxIgnoreKeywords->isChecked();
    m_settings.checkAllCapsWords = !ui->checkBoxIgnoreCaps->isChecked();
    m_settings.removeWordsThatAppearInSource = ui->checkBoxWordsInSource->isChecked();
    m_settings.removeWebsites = ui->checkBoxWebsiteAddresses->isChecked();
    return m_settings;
}
//--------------------------------------------------

void CppParserOptionsWidget::radioButtonNumbersToggled()
{
    if(static_cast<QRadioButton*>(sender())->isChecked() == true) {
        m_settings.wordsWithNumberOption = static_cast<CppParserSettings::WordsWithNumbersOption>(sender()->property(ENUM_VAL_PROPERTY).toInt());
    }
}
//--------------------------------------------------

void CppParserOptionsWidget::radioButtonUnderscoresToggled()
{
    if(static_cast<QRadioButton*>(sender())->isChecked() == true) {
        m_settings.wordsWithUnderscoresOption = static_cast<CppParserSettings::WordsWithUnderscoresOption>(sender()->property(ENUM_VAL_PROPERTY).toInt());
    }
}
//--------------------------------------------------

void CppParserOptionsWidget::radioButtonCamelCaseToggled()
{
    if(static_cast<QRadioButton*>(sender())->isChecked() == true) {
        m_settings.camelCaseWordOption = static_cast<CppParserSettings::CamelCaseWordOption>(sender()->property(ENUM_VAL_PROPERTY).toInt());
    }
}
//--------------------------------------------------

void CppParserOptionsWidget::radioButtonDotsToggled()
{
    if(static_cast<QRadioButton*>(sender())->isChecked() == true) {
        m_settings.wordsWithDotsOption = static_cast<CppParserSettings::WordsWithDotsOption>(sender()->property(ENUM_VAL_PROPERTY).toInt());
    }
}
//--------------------------------------------------

void CppParserOptionsWidget::updateWithSettings(const CppParserSettings *const settings)
{
    ui->checkBoxRemoveEmailAddresses->setChecked(settings->removeEmailAddresses);
    ui->checkBoxIgnoreKeywords->setChecked(!settings->checkQtKeywords);
    ui->checkBoxIgnoreCaps->setChecked(!settings->checkAllCapsWords);
    QRadioButton* numberButtons[] = {ui->radioButtonNumbersRemove, ui->radioButtonNumbersSplit, ui->radioButtonNumbersLeave};
    numberButtons[settings->wordsWithNumberOption]->setChecked(true);
    QRadioButton* underscoreButtons[] = {ui->radioButtonUnderscoresRemove, ui->radioButtonUnderscoresSplit, ui->radioButtonUnderscoresLeave};
    underscoreButtons[settings->wordsWithUnderscoresOption]->setChecked(true);
    QRadioButton* camelCaseButtons[] = {ui->radioButtonCamelRemove, ui->radioButtonCamelSplit, ui->radioButtonCamelLeave};
    camelCaseButtons[settings->camelCaseWordOption]->setChecked(true);
    ui->checkBoxWordsInSource->setChecked(settings->removeWordsThatAppearInSource);
    QRadioButton* dotsButtons[] = {ui->radioButtonDotsRemove, ui->radioButtonDotsSplit, ui->radioButtonDotsLeave};
    dotsButtons[settings->wordsWithDotsOption]->setChecked(true);
    ui->checkBoxWebsiteAddresses->setChecked(settings->removeWebsites);
}
//--------------------------------------------------

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker
