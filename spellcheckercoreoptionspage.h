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

#ifndef SPELLCHECKER_INTERNAL_SPELLCHECKERCOREOPTIONSPAGE_H
#define SPELLCHECKER_INTERNAL_SPELLCHECKERCOREOPTIONSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace SpellChecker {
namespace Internal {

class SpellCheckerCoreSettings;
class SpellCheckerCoreOptionsWidget;

class SpellCheckerCoreOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    SpellCheckerCoreOptionsPage(SpellCheckerCoreSettings* settings, QObject *parent = 0);
    virtual ~SpellCheckerCoreOptionsPage();

    bool matches(const QString &searchKeyWord) const;
    QWidget *widget();
    void apply();
    void finish();
signals:

public slots:
private:
    SpellCheckerCoreSettings* const m_settings;
    QPointer<SpellCheckerCoreOptionsWidget> m_widget;
};

} // namespace Internal
} // namespace SpellChecker


#endif // SPELLCHECKER_INTERNAL_SPELLCHECKERCOREOPTIONSPAGE_H
