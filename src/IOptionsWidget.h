/**************************************************************************
**
** Copyright (c) 2023 Carel Combrink
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

#include <QWidget>

namespace SpellChecker {

/**
 * @brief Base class for an options widget.
 */
class IOptionsWidget : public QWidget {

  Q_OBJECT
public:
  IOptionsWidget() = default;
  ~IOptionsWidget() override = default;

  /**
   * @brief Apply the settings on the widget.
   *
   * This function is called from the Core Options page when
   * settings must be applied. The implementation can assume that
   * widget parent is still valid when this function is called.
   *
   */
  virtual void apply() = 0;

signals:
  /**
   * @brief Signal that the widget can emit on errors.
   *
   * The error is displayed on the core options widget.
   *
   * @param spellcheckerName Name of the spellchecker where the error comes from.
   * @param errorString The message describing the error.
   */
  void optionsError(const QString &spellcheckerName,
                    const QString &errorString);
};

} // namespace SpellChecker
