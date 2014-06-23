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

#ifndef SPELLCHECKER_INTERNAL_SUGGESTIONSDIALOG_H
#define SPELLCHECKER_INTERNAL_SUGGESTIONSDIALOG_H

#include <QDialog>
#include <QModelIndex>

namespace SpellChecker {
namespace Internal {

namespace Ui {
class SuggestionsDialog;
}

class SuggestionsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SuggestionsDialog(const QString& word, const QStringList& suggestions, QWidget *parent = 0);
    QString replacementWord() const;
    ~SuggestionsDialog();
    
private slots:
    void on_listWidgetSuggestions_doubleClicked(const QModelIndex &index);
    void on_lineEditReplacement_textChanged(const QString &arg1);
    void on_listWidgetSuggestions_currentTextChanged(const QString &currentText);

private:
    Ui::SuggestionsDialog *ui;
};


} // namespace Internal
} // namespace SpellChecker
#endif // SPELLCHECKER_INTERNAL_SUGGESTIONSDIALOG_H
