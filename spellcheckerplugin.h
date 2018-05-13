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
#include "spellcheckerconstants.h"
#include "spellcheckercore.h"
#include "spellcheckquickfix.h"
#include "outputpane.h"
#include "NavigationWidget.h"

/* SpellCheckers */
#include "SpellCheckers/HunspellChecker/hunspellchecker.h"


#include <memory>

#include <extensionsystem/iplugin.h>

/* Parsers */
#include "Parsers/CppParser/cppdocumentparser.h"
#include "Parsers/CppParser/cppparsersettings.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <texteditor/texteditorconstants.h>

namespace SpellChecker {
class SpellCheckerCore;

namespace CppSpellChecker {
class Word;
namespace Internal {
class CppParserSettings;
} /* Internal */
} /* CppSpellChecker */


namespace Internal {

class SpellCheckerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "SpellChecker.json")

public:
    SpellCheckerPlugin();
    ~SpellCheckerPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();
private:
    std::unique_ptr<NavigationWidgetFactory> m_navigationWidgetFactory;
    std::unique_ptr<SpellCheckCppQuickFixFactory> m_spellCheckCppQuickFixFactory;

    std::unique_ptr<SpellChecker::CppSpellChecker::Internal::CppDocumentParser> m_cppParser;

    std::unique_ptr<SpellCheckerCore> m_spellCheckerCore;
    std::unique_ptr<CppSpellChecker::Internal::CppParserSettings> m_cppParserSettings;
    std::unique_ptr<SpellChecker::Checker::Hunspell::HunspellChecker> m_spellChecker;
};

} // namespace Internal
} // namespace SpellChecker
