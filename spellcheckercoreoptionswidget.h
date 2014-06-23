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

#ifndef SPELLCHECKERCOREOPTIONSWIDGET_H
#define SPELLCHECKERCOREOPTIONSWIDGET_H

#include "spellcheckercoresettings.h"

#include <QWidget>

namespace SpellChecker {
namespace Internal {

namespace Ui {
class SpellCheckerCoreOptionsWidget;
}

class SpellCheckerCoreOptionsWidget : public QWidget
{
    Q_OBJECT

public:
    SpellCheckerCoreOptionsWidget(const SpellCheckerCoreSettings* const settings, QWidget *parent = 0);
    ~SpellCheckerCoreOptionsWidget();

    const SpellCheckerCoreSettings& settings();

private slots:
    void on_comboBoxSpellChecker_currentIndexChanged(const QString &arg1);

private:
    void updateWithSettings(const SpellCheckerCoreSettings* const settings);
    Ui::SpellCheckerCoreOptionsWidget *ui;
    SpellCheckerCoreSettings m_settings;
};

} // Internal
} // SpellChecker

#endif // SPELLCHECKERCOREOPTIONSWIDGET_H
