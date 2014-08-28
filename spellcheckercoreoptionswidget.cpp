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
    /* Hide the error widget by default, since there should not be errors. */
    ui->widgetErrorOutput->setVisible(false);

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
    m_settings.activeSpellChecker   = ui->comboBoxSpellChecker->currentText();
    m_settings.onlyParseCurrentFile = ui->checkBoxOnlyCheckCurrent->isChecked();
    return m_settings;
}
//--------------------------------------------------

void SpellCheckerCoreOptionsWidget::applySettings()
{
    ui->labelError->clear();
    ui->widgetErrorOutput->setVisible(false);
    emit applyCurrentSetSettings();
}
//--------------------------------------------------

void SpellCheckerCoreOptionsWidget::optionsPageError(const QString &optionsPage, const QString &error)
{
    QString displayString = QString(QLatin1String("<b>%1</b>: %2")).arg(optionsPage, error);
    ui->labelError->setText(displayString);
    ui->widgetErrorOutput->setVisible(true);
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
    ui->checkBoxOnlyCheckCurrent->setChecked(settings->onlyParseCurrentFile);
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
    connect(this, SIGNAL(applyCurrentSetSettings()), widget, SLOT(applySettings()));
    connect(widget, SIGNAL(optionsError(QString,QString)), this, SLOT(optionsPageError(QString,QString)));
}
//--------------------------------------------------
