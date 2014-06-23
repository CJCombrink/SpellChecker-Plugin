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

#include "hunspelloptionswidget.h"
#include "ui_hunspelloptionswidget.h"

#include <QFileDialog>

using namespace SpellChecker::Checker::Hunspell;

HunspellOptionsWidget::HunspellOptionsWidget(const QString &dictionary, const QString &userDictionary, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HunspellOptionsWidget)
{
    ui->setupUi(this);
    updateDictionary(dictionary);
    updateUserDictionary(userDictionary);

    connect(ui->lineEditDictionary, SIGNAL(textChanged(QString)), this, SIGNAL(dictionaryChanged(QString)));
    connect(ui->lineEditUserDictionary, SIGNAL(textChanged(QString)), this, SIGNAL(userDictionaryChanged(QString)));
}
//--------------------------------------------------

HunspellOptionsWidget::~HunspellOptionsWidget()
{
    delete ui;
}
//--------------------------------------------------

void HunspellOptionsWidget::updateDictionary(const QString &dictionary)
{
    ui->lineEditDictionary->setText(dictionary);
}
//--------------------------------------------------

void HunspellOptionsWidget::updateUserDictionary(const QString &userDictionary)
{
    ui->lineEditUserDictionary->setText(userDictionary);
}
//--------------------------------------------------

void HunspellOptionsWidget::on_toolButtonBrowseDictionary_clicked()
{
    QString dictionary = QFileDialog::getOpenFileName(this,
                                                      tr("Dictionary File"),
                                                      ui->lineEditDictionary->text(),
                                                      tr("Dictionaries (*.dic)"),
                                                      0,
                                                      QFileDialog::ReadOnly);
    updateDictionary(dictionary);
}
//--------------------------------------------------

void HunspellOptionsWidget::on_toolButtonBrowseUserDictionary_clicked()
{
    QString userDictionary = QFileDialog::getSaveFileName(this,
                                                      tr("User Dictionary File"),
                                                      ui->lineEditUserDictionary->text(),
                                                      tr("Dictionaries (*.udic)"),
                                                      0,
                                                      0);
    updateUserDictionary(userDictionary);
}
//--------------------------------------------------
