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

#include <coreplugin/icore.h>

#include <QFileDialog>

using namespace SpellChecker::Checker::Hunspell;

HunspellOptionsWidget::HunspellOptionsWidget(const QString &dictionary, const QString &userDictionary, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HunspellOptionsWidget)
{
    ui->setupUi(this);
    /* Set the hints on the different Dictionaries */
    ui->lineEditDictionary->setToolTip(tr("The dictionary is a *.dic file that Hunspell will use to spellcheck words. \n"
                                          "A *.aff file with the same name as the selected dictionary file must be in the selected folder. \n"
                                          "On most Unix/ Linux based system some will already be installed and can be re-used. \n"
                                          "On other systems one might need to download one manually."));
    ui->lineEditUserDictionary->setToolTip(tr("The User Dictionary is a custom user dictionary that the Hunspell Spellchecker \n"
                                              "will use to remember words that get added to the dictionary. \n"
                                              "If such a file does not already exist, it will get created with the given information. "));

    updateDictionary(dictionary);
    updateUserDictionary(userDictionary);
}
//--------------------------------------------------

HunspellOptionsWidget::~HunspellOptionsWidget()
{
    delete ui;
}
//--------------------------------------------------

void HunspellOptionsWidget::applySettings()
{
    /* Make sure the dictionaries exist */
    QFileInfo dict(ui->lineEditDictionary->text());
    if(dict.exists() == true) {
        emit dictionaryChanged(ui->lineEditDictionary->text());
    } else {
        emit optionsError(QLatin1String("Hunspell Spellchecker"), tr("Dictionary does not exist"));
        return;
    }
    /* Check if the aff file is located along with the .dic file */
    QString affFIleName = QString(ui->lineEditDictionary->text()).replace(QRegExp(QLatin1String("\\.dic$")), QLatin1String(".aff"));
    QFileInfo affFile(affFIleName);
    if(affFile.exists() == false) {
        emit optionsError(QLatin1String("Hunspell Spellchecker"), tr("The *.aff File for selected dictionary does not exist"));
        return;
    }

    QFileInfo userDict(ui->lineEditUserDictionary->text());
    if(!userDict.dir().mkpath(".")) {
        emit optionsError(QLatin1String("Hunspell Spellchecker"), tr("Path to user dictionary could not be created"));
        return;
    }
    /* The Dir should exist at this point, check if the file exists and can be made if it does not */
    if(userDict.exists() == false) {
        QFile file(ui->lineEditUserDictionary->text());
        if(file.open(QFile::ReadWrite | QFile::Text) == false) {
            emit optionsError(QLatin1String("Hunspell Spellchecker"), tr("User dictionary can not be created, perhaps insufficient access on folder."));
            return;
        }
    }
    /* At this point the user dictionary specified should be valid. */
    emit userDictionaryChanged(ui->lineEditUserDictionary->text());
}
//--------------------------------------------------

void HunspellOptionsWidget::updateDictionary(const QString &dictionary)
{
    ui->lineEditDictionary->setText(dictionary);
    /* If the dictionary gets changed, and the user dictionary is empty
     * Create a self generated user dictionary name derived from the
     * selected dictionary. This is done to create a betters set-up experience. */
    if((ui->lineEditUserDictionary->text().isEmpty() == true)
            && (dictionary.isEmpty() == false)){
        /* Create a temp name in the user resource path, on windows this will be %APPDATA% */
        QFileInfo fileInfo(dictionary);
        QString userDictFileName = fileInfo.fileName();
        userDictFileName.replace(QRegularExpression(QLatin1String("\\.dic$")), QLatin1String(".udic"));
        userDictFileName = QLatin1String("QtC-") + userDictFileName;
        /* Add the file name to the User Resource Path */
        QString userDictName = Core::ICore::userResourcePath() + QLatin1String("/UserDictionaries/") + userDictFileName;
        updateUserDictionary(userDictName);
    }
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
    if(dictionary.isEmpty() == false) {
        updateDictionary(dictionary);
    }
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
    if(userDictionary.isEmpty() == false) {
        updateUserDictionary(userDictionary);
    }
}
//--------------------------------------------------
