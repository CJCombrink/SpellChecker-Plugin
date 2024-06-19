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

#pragma once

#include <QWidget>
#include "cppparsersettings.h"

namespace SpellChecker {
namespace CppSpellChecker {
namespace Internal {

class CppParserSettings;

namespace Ui {
class CppParserOptionsWidget;
} // namespace Ui

class CppParserOptionsWidget
  : public QWidget
{
  Q_OBJECT

public:
  CppParserOptionsWidget( const CppParserSettings* const settings, QWidget* parent = 0 );
  ~CppParserOptionsWidget();

  const CppParserSettings& settings();

public slots:
  void checkBoxWhatToggled();
  void radioButtonCommentsToggled();
  void radioButtonNumbersToggled();
  void radioButtonUnderscoresToggled();
  void radioButtonCamelCaseToggled();
  void radioButtonDotsToggled();

private:
  void updateWithSettings( const CppParserSettings* const settings );
  Ui::CppParserOptionsWidget* ui;
  CppParserSettings m_settings;
};


} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker
