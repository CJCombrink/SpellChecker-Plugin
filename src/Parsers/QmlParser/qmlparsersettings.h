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

#pragma once

#include <utils/qtcsettings.h>

#include <QObject>

namespace SpellChecker {
namespace QmlSpellChecker {
namespace Internal {

/*! \brief Settings for the QML parser.
 *
 * These settings control what QML token types are checked and which common
 * false positives are removed before words are passed to the spell checker.
 */
class QmlParserSettings
  : public QObject
{
  Q_OBJECT
  Q_ENUMS( WhatToCheck )
  Q_FLAGS( WhatToCheckOptions )
public:
  QmlParserSettings();
  QmlParserSettings( const QmlParserSettings& settings );
  ~QmlParserSettings() override;

  /*! \brief QML token types that should be spell checked. */
  enum WhatToCheck {
    Tokens_NONE         = 0,                              /*!< Nothing should be checked. */
    CheckComments       = 1 << 0,                         /*!< Check QML line and block comments. */
    CheckStringLiterals = 1 << 1,                         /*!< Check QML string and template literals. */
    CheckBoth           = CheckComments | CheckStringLiterals /*!< Check comments and literals. */
  };
  Q_DECLARE_FLAGS( WhatToCheckOptions, WhatToCheck )

  WhatToCheckOptions whatToCheck; /*!< Token types to parse. */
  bool removeEmailAddresses;      /*!< Remove email addresses before spell checking. */
  bool checkAllCapsWords;         /*!< Keep all-caps words when true. */
  bool removeWebsites;            /*!< Remove website addresses before spell checking. */

  /*! \brief Load settings from Qt Creator settings.
   * \param[in] settings Qt Creator settings object. */
  void loadFromSettings( Utils::QtcSettings* settings );
  /*! \brief Save settings to Qt Creator settings.
   * \param[in] settings Qt Creator settings object. */
  void saveToSettings( Utils::QtcSettings* settings ) const;

  QmlParserSettings& operator=( const QmlParserSettings& other );
  bool operator==( const QmlParserSettings& other ) const;

signals:
  /*! \brief Emitted when assignment changes one or more settings. */
  void settingsChanged();

private:
  /*! \brief Reset all values to defaults. */
  void setDefaults();
};

} // namespace Internal
} // namespace QmlSpellChecker
} // namespace SpellChecker

Q_DECLARE_OPERATORS_FOR_FLAGS( SpellChecker::QmlSpellChecker::Internal::QmlParserSettings::WhatToCheckOptions )
