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

#include "suggestionsdialog.h"
#include "ui_suggestionsdialog.h"

#include <QPushButton>

using namespace SpellChecker::Internal;

SuggestionsDialog::SuggestionsDialog(const QString &word, const QStringList &suggestions, qint32 occurrences, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SuggestionsDialog)
{
    ui->setupUi(this);
    ui->lineEditWord->setText(word);
    ui->listWidgetSuggestions->addItems(suggestions);
    ui->listWidgetSuggestions->setFocus();
    if(suggestions.count() > 0) {
        ui->lineEditReplacement->setText(suggestions.front());
    }

    if(occurrences > 1) {
        ui->pushButtonReplaceAll->setText(ui->pushButtonReplaceAll->text().replace(QLatin1String("xxx"), QString::number(occurrences)));
    } else {
        ui->pushButtonReplaceAll->setVisible(false);
    }
}
//--------------------------------------------------

SuggestionsDialog::~SuggestionsDialog()
{
    delete ui;
}
//--------------------------------------------------

QString SuggestionsDialog::replacementWord() const
{
    Q_ASSERT(ui->lineEditReplacement->text().isEmpty() == false);
    return ui->lineEditReplacement->text();
}
//--------------------------------------------------

void SpellChecker::Internal::SuggestionsDialog::on_listWidgetSuggestions_doubleClicked(const QModelIndex &index)
{
    if(index.isValid() == false) {
        return;
    }
    /* Text Change should have happened already, so no reason to set the selection again */
    Q_ASSERT(ui->lineEditReplacement->text() == index.data(Qt::DisplayRole).toString());
    accept();
}
//--------------------------------------------------

void SpellChecker::Internal::SuggestionsDialog::on_lineEditReplacement_textChanged(const QString &arg1)
{
    /* Only enable the Ok button, when there is valid text to replace the
     * word with */
    ui->pushButtonReplace->setEnabled(arg1.isEmpty() == false);
    ui->pushButtonReplaceAll->setEnabled(arg1.isEmpty() == false);
}
//--------------------------------------------------

void SpellChecker::Internal::SuggestionsDialog::on_listWidgetSuggestions_currentTextChanged(const QString &currentText)
{
    Q_ASSERT(currentText.isEmpty() == false);
    ui->lineEditReplacement->setText(currentText);
}
//--------------------------------------------------

void SpellChecker::Internal::SuggestionsDialog::on_pushButtonReplace_clicked()
{
    accept();
}
//--------------------------------------------------

void SpellChecker::Internal::SuggestionsDialog::on_pushButtonReplaceAll_clicked()
{
    done(AcceptAll);
}
//--------------------------------------------------

void SpellChecker::Internal::SuggestionsDialog::on_pushButtonCancel_clicked()
{
    reject();
}
//--------------------------------------------------
