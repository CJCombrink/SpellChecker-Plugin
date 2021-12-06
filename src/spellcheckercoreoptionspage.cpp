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

#include "spellcheckerconstants.h"
#include "spellcheckercoreoptionspage.h"
#include "spellcheckercoreoptionswidget.h"
#include "spellcheckercoresettings.h"

using namespace SpellChecker::Internal;

SpellCheckerCoreOptionsPage::SpellCheckerCoreOptionsPage( SpellCheckerCoreSettings* settings, QObject* parent )
  : Core::IOptionsPage( parent )
  , m_settings( settings )
{
  setId( "SpellChecker::CoreSettings" );
  setDisplayName( tr( "SpellChecker" ) );
  setCategory( "SpellChecker" );
  setDisplayCategory( tr( "Spell Checker" ) );
  setCategoryIcon( Utils::Icon( Constants::ICON_SPELLCHECKERPLUGIN_OPTIONS ) );
}
// --------------------------------------------------

SpellCheckerCoreOptionsPage::~SpellCheckerCoreOptionsPage()
{}
// --------------------------------------------------

bool SpellCheckerCoreOptionsPage::matches( const QString& searchKeyWord ) const
{
  return ( searchKeyWord == QLatin1String( "SpellChecker" ) );
}
// --------------------------------------------------

QWidget* SpellCheckerCoreOptionsPage::widget()
{
  if( m_widget == nullptr ) {
    m_widget = new SpellCheckerCoreOptionsWidget( m_settings );
  }
  return m_widget;
}
// --------------------------------------------------

void SpellCheckerCoreOptionsPage::apply()
{
  Q_ASSERT( m_widget != nullptr );
  m_widget->applySettings();
  *m_settings = m_widget->settings();
}
// --------------------------------------------------

void SpellCheckerCoreOptionsPage::finish()
{}
// --------------------------------------------------
