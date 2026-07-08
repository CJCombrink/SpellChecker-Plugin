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

#include "qmlparseroptionswidget.h"
#include "qmlparsersettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace SpellChecker {
namespace QmlSpellChecker {
namespace Internal {

/*! \brief Exposes QML spell-checking settings in Qt Creator's options dialog.
 *
 * This class connects the persistent QML parser settings to the options
 * widget managed by Qt Creator.
 */
class QmlParserOptionsPage
  : public Core::IOptionsPage
{
public:
  /*! \brief Constructor.
   * \param[in] settings Settings object edited by this page. */
  explicit QmlParserOptionsPage( QmlParserSettings* settings );
  ~QmlParserOptionsPage() override;

private:
  QmlParserSettings* m_settings;             /*!< Settings object edited by the page. */
  QmlParserOptionsWidget* m_widget = nullptr;/*!< Lazily-created page widget. */
};

} // namespace Internal
} // namespace QmlSpellChecker
} // namespace SpellChecker
