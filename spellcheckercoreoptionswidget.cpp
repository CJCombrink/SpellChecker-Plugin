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

#include "spellcheckercoreoptionswidget.h"
#include "ui_spellcheckercoreoptionswidget.h"
#include "spellcheckercoresettings.h"
#include "spellcheckercore.h"
#include "ISpellChecker.h"

using namespace SpellChecker::Internal;

SpellCheckerCoreOptionsWidget::SpellCheckerCoreOptionsWidget(const SpellChecker::Internal::SpellCheckerCoreSettings * const settings, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SpellCheckerCoreOptionsWidget)
{
    ui->setupUi(this);


    ui->comboBoxSpellChecker->addItem(QLatin1String(""));
    QMap<QString, ISpellChecker*> availableSpellCheckers = SpellCheckerCore::instance()->addedSpellCheckers();
    foreach(const QString& name, availableSpellCheckers.keys()) {
        ui->comboBoxSpellChecker->addItem(name);
    }

    updateWithSettings(settings);
}
//--------------------------------------------------

SpellCheckerCoreOptionsWidget::~SpellCheckerCoreOptionsWidget()
{
    delete ui;
}
//--------------------------------------------------

const SpellCheckerCoreSettings &SpellCheckerCoreOptionsWidget::settings()
{
    m_settings.activeSpellChecker = ui->comboBoxSpellChecker->currentText();
    return m_settings;
}
//--------------------------------------------------

void SpellCheckerCoreOptionsWidget::updateWithSettings(const SpellCheckerCoreSettings * const settings)
{
    int index = ui->comboBoxSpellChecker->findText(settings->activeSpellChecker);
    if(index == -1) {
        qDebug() << "Spellchecker from settings not valid option: " << settings->activeSpellChecker;
        Q_ASSERT_X(false, "updateWithSettings",  "Spellchecker from settings not valid option: " + settings->activeSpellChecker.toAscii());
        return;
    }
    ui->comboBoxSpellChecker->setCurrentIndex(index);
}
//--------------------------------------------------

void SpellChecker::Internal::SpellCheckerCoreOptionsWidget::on_comboBoxSpellChecker_currentIndexChanged(const QString &arg1)
{
    delete ui->spellCheckerOptionsWidgetHolder->layout();
    QVBoxLayout* layout = new QVBoxLayout(ui->spellCheckerOptionsWidgetHolder);
    layout->setMargin(0);

    if(arg1.isEmpty() == true) {
        return;
    }

    QMap<QString, ISpellChecker*> availableSpellCheckers = SpellCheckerCore::instance()->addedSpellCheckers();
    if(availableSpellCheckers.contains(arg1) == false) {
        return;
    }
    QWidget* widget = availableSpellCheckers[arg1]->optionsWidget();
    layout->addWidget(widget);
}
//--------------------------------------------------
