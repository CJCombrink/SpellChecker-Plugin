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

#include "spellchecker_global.h"

#include <extensionsystem/iplugin.h>

namespace SpellChecker {
class SpellCheckerCore;

namespace CppSpellChecker {
class Word;
namespace Internal {
class CppParserSettings;
} /* Internal */
} /* CppSpellChecker */


namespace Internal {
class SpellCheckerPluginPrivate;
class SpellCheckerPlugin
  : public ExtensionSystem::IPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA( IID "org.qt-project.Qt.QtCreatorPlugin" FILE "SpellChecker.json" )

public:
  SpellCheckerPlugin();
  ~SpellCheckerPlugin();

  Utils::Result<> initialize(const QStringList& arguments);
  void extensionsInitialized();
  ShutdownFlag aboutToShutdown();
private:
  SpellCheckerPluginPrivate* const d;
};

} // namespace Internal
} // namespace SpellChecker
