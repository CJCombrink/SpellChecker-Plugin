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

#include "qmlparsersettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace SpellChecker {
namespace QmlSpellChecker {
namespace Internal {

/*! \brief Provides controls for configuring QML spell checking.
 *
 * This class edits which QML text is checked and which common false-positive
 * patterns are ignored.
 */
class QmlParserOptionsWidget
  : public Core::IOptionsPageWidget
{
  Q_OBJECT
public:
  /*! \brief Constructor.
   * \param[in] settings Initial settings used to populate the controls. */
  explicit QmlParserOptionsWidget( QmlParserSettings* settings );
  ~QmlParserOptionsWidget() override;

  /*! \brief Collect current widget state into a settings object.
   * \return Settings represented by the current UI state. */
  const QmlParserSettings& settings();

private:
  /*! \brief Populate controls from settings.
   * \param[in] settings Settings to show. */
  void updateWithSettings( const QmlParserSettings* settings );

  QmlParserSettings m_settings;      /*!< Cached settings returned by settings(). */
  QCheckBox* m_checkComments;        /*!< Toggle for checking comments. */
  QCheckBox* m_checkStringLiterals;  /*!< Toggle for checking string literals. */
  QCheckBox* m_ignoreAllCaps;        /*!< Toggle for removing all-caps words. */
  QCheckBox* m_removeEmailAddresses; /*!< Toggle for removing email addresses. */
  QCheckBox* m_removeWebsites;       /*!< Toggle for removing website addresses. */
};

} // namespace Internal
} // namespace QmlSpellChecker
} // namespace SpellChecker
