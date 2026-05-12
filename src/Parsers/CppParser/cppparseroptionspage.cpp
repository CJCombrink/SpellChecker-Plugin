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

#include "cppparseroptionspage.h"
#include "cppparseroptionswidget.h"
#include "cppparsersettings.h"

using namespace SpellChecker::CppSpellChecker::Internal;

CppParserOptionsPage::CppParserOptionsPage( CppParserSettings* settings )
  : Core::IOptionsPage( )
  , m_settings( settings )
{
  setId( "SpellChecker::CppDocumentParserSettings" );
  setDisplayName( CppParserOptionsWidget::tr( "C++ Parser" ) );
  setCategory( "SpellChecker" );
  registerCategory("SpellChecker",
                   CppParserOptionsWidget::tr( "Spell Checker" ),
                   ":/spellcheckerplugin/images/optionspageicon_solid.png");
  setWidgetCreator([this]() -> Core::IOptionsPageWidget *{
    if( m_widget == nullptr ) {
      m_widget = new CppParserOptionsWidget( m_settings );
      m_widget->setOnApply([this](){
        *m_settings = m_widget->settings();
      });
      m_widget->setDirtyChecker([this](){ return  *m_settings != m_widget->settings(); });
    }
    return m_widget;
  });
  setFixedKeywords({ QLatin1String( "SpellChecker" ) });
}
// --------------------------------------------------

CppParserOptionsPage::~CppParserOptionsPage()
{}
// --------------------------------------------------
