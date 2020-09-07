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

#include "../../ISpellChecker.h"

#include <QObject>

namespace SpellChecker {
namespace Checker {
namespace Hunspell {

class HunspellCheckerPrivate;
class HunspellChecker
  : public SpellChecker::ISpellChecker
{
  Q_OBJECT
public:
  HunspellChecker();
  ~HunspellChecker() override;

  QString name() const Q_DECL_OVERRIDE;
  bool isSpellingMistake( const QString& word ) const Q_DECL_OVERRIDE;
  void getSuggestionsForWord( const QString& word, QStringList& suggestionsList ) const Q_DECL_OVERRIDE;
  bool addWord( const QString& word ) Q_DECL_OVERRIDE;
  bool ignoreWord( const QString& word ) Q_DECL_OVERRIDE;
  QWidget* optionsWidget() Q_DECL_OVERRIDE;

signals:
  void dictionaryChanged( const QString& dictionary );
  void userDictionaryChanged( const QString& userDictionary );

public slots:
  void updateDictionary( const QString& dictionary );
  void updateUserDictionary( const QString& userDictionary );

private:
  void loadSettings();
  void saveSettings() const;
  void loadUserAddedWords();
  HunspellCheckerPrivate* const d;
};

} // namespace Hunspell
} // namespace Checker
} // namespace SpellChecker
