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

#include <QObject>
#include <QSettings>

namespace SpellChecker {
namespace Internal {

class SpellCheckerCoreSettings
  : public QObject
{
  Q_OBJECT
public:
  SpellCheckerCoreSettings();
  SpellCheckerCoreSettings( const SpellCheckerCoreSettings& settings );
  ~SpellCheckerCoreSettings();

  void saveToSettings( QSettings* settings ) const;
  void loadFromSettings( QSettings* settings );

  SpellCheckerCoreSettings& operator=( const SpellCheckerCoreSettings& other );
  bool operator==( const SpellCheckerCoreSettings& other ) const;

  QString activeSpellChecker;
  bool onlyParseCurrentFile;
  bool checkExternalFiles;
  QStringList projectsToIgnore;
  /*! Replace all occurrences of a misspelled word on the current page when
   * a suggestion is selected from the right click menu. */
  bool replaceAllFromRightClick;

signals:
  void settingsChanged();

};

} // namespace Internal
} // namespace SpellChecker
