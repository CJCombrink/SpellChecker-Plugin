/**************************************************************************
**
** Copyright (c) 2026 Marcel Petrick
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

#include "qmlparseroptionswidget.h"

#include <utils/qtcsettings.h>

#include <QCheckBox>
#include <QVBoxLayout>

using namespace SpellChecker::QmlSpellChecker::Internal;

QmlParserOptionsWidget::QmlParserOptionsWidget( QmlParserSettings* settings )
{
  auto layout = new QVBoxLayout( this );

  m_checkComments = new QCheckBox( tr( "Check comments" ), this );
  m_checkStringLiterals = new QCheckBox( tr( "Check string literals" ), this );
  m_ignoreAllCaps = new QCheckBox( tr( "Ignore all-caps words" ), this );
  m_removeEmailAddresses = new QCheckBox( tr( "Ignore email addresses" ), this );
  m_removeWebsites = new QCheckBox( tr( "Ignore website addresses" ), this );

  layout->addWidget( m_checkComments );
  layout->addWidget( m_checkStringLiterals );
  layout->addWidget( m_ignoreAllCaps );
  layout->addWidget( m_removeEmailAddresses );
  layout->addWidget( m_removeWebsites );
  layout->addStretch( 1 );

  updateWithSettings( settings );

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
  connect( m_checkComments, &QCheckBox::checkStateChanged, this, [](){ Utils::markSettingsDirty(); } );
  connect( m_checkStringLiterals, &QCheckBox::checkStateChanged, this, [](){ Utils::markSettingsDirty(); } );
  connect( m_ignoreAllCaps, &QCheckBox::checkStateChanged, this, [](){ Utils::markSettingsDirty(); } );
  connect( m_removeEmailAddresses, &QCheckBox::checkStateChanged, this, [](){ Utils::markSettingsDirty(); } );
  connect( m_removeWebsites, &QCheckBox::checkStateChanged, this, [](){ Utils::markSettingsDirty(); } );
#else
  connect( m_checkComments, &QCheckBox::stateChanged, this, [](){ Utils::markSettingsDirty(); } );
  connect( m_checkStringLiterals, &QCheckBox::stateChanged, this, [](){ Utils::markSettingsDirty(); } );
  connect( m_ignoreAllCaps, &QCheckBox::stateChanged, this, [](){ Utils::markSettingsDirty(); } );
  connect( m_removeEmailAddresses, &QCheckBox::stateChanged, this, [](){ Utils::markSettingsDirty(); } );
  connect( m_removeWebsites, &QCheckBox::stateChanged, this, [](){ Utils::markSettingsDirty(); } );
#endif
}

QmlParserOptionsWidget::~QmlParserOptionsWidget() = default;

const QmlParserSettings& QmlParserOptionsWidget::settings()
{
  QmlParserSettings::WhatToCheckOptions whatToCheck = QmlParserSettings::Tokens_NONE;
  if( m_checkComments->isChecked() == true ) {
    whatToCheck |= QmlParserSettings::CheckComments;
  }
  if( m_checkStringLiterals->isChecked() == true ) {
    whatToCheck |= QmlParserSettings::CheckStringLiterals;
  }

  m_settings.whatToCheck = whatToCheck;
  m_settings.checkAllCapsWords = !m_ignoreAllCaps->isChecked();
  m_settings.removeEmailAddresses = m_removeEmailAddresses->isChecked();
  m_settings.removeWebsites = m_removeWebsites->isChecked();
  return m_settings;
}

void QmlParserOptionsWidget::updateWithSettings( const QmlParserSettings* settings )
{
  m_checkComments->setChecked( settings->whatToCheck.testFlag( QmlParserSettings::CheckComments ) );
  m_checkStringLiterals->setChecked( settings->whatToCheck.testFlag( QmlParserSettings::CheckStringLiterals ) );
  m_ignoreAllCaps->setChecked( !settings->checkAllCapsWords );
  m_removeEmailAddresses->setChecked( settings->removeEmailAddresses );
  m_removeWebsites->setChecked( settings->removeWebsites );
}
