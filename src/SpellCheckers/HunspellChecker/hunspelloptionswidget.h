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

namespace SpellChecker {
namespace Checker {
namespace Hunspell {

namespace Ui {
class HunspellOptionsWidget;
} // namespace Ui

class HunspellOptionsWidget
  : public QWidget
{
  Q_OBJECT

public:
  HunspellOptionsWidget( const QString& dictionary, const QString& userDictionary, QWidget* parent = 0 );
  ~HunspellOptionsWidget();

signals:
  void optionsError( const QString& spellcheckerName, const QString& errorString );
  void dictionaryChanged( const QString& dictionary );
  void userDictionaryChanged( const QString& userDictionary );

public slots:
  void applySettings();
  void updateDictionary( const QString& dictionary );
  void updateUserDictionary( const QString& userDictionary );

private slots:
  void on_toolButtonBrowseDictionary_clicked();
  void on_toolButtonBrowseUserDictionary_clicked();

private:
  Ui::HunspellOptionsWidget* ui;
};


} // namespace Hunspell
} // namespace Checker
} // namespace SpellChecker
