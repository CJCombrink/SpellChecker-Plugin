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

#include "spellcheckerplugin.h"

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>

using namespace SpellChecker;
using namespace SpellChecker::Internal;

/* The following define can be used to install the myMessageOutput function as the
 * a Qt Message Handler. This is useful for debugging when there is a need to break
 * the application based on some text output from the application. */
//#define DEBUG_INSTALL_MESSAGE_HANDLER
#ifdef DEBUG_INSTALL_MESSAGE_HANDLER
// To debug a function: __PRETTY_FUNCTION__
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}
#endif /* DEBUG_INSTALL_MESSAGE_HANDLER */

SpellCheckerPlugin::SpellCheckerPlugin() 
{
    qRegisterMetaType<SpellChecker::WordList>("SpellChecker::WordList");
}

SpellCheckerPlugin::~SpellCheckerPlugin()
{
    delete m_spellCheckerCore->outputPane ();
    delete m_spellCheckerCore->optionsPage ();
    delete m_cppParser->optionsPage ();
}

bool SpellCheckerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize function, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    Q_UNUSED(arguments)
    Q_UNUSED(errorString)
#ifdef DEBUG_INSTALL_MESSAGE_HANDLER
    qInstallMessageHandler(myMessageOutput);
#endif /* DEBUG_INSTALL_MESSAGE_HANDLER */
    
    m_spellCheckerCore.reset (new SpellCheckerCore(this));

    Core::Context textContext(TextEditor::Constants::C_TEXTEDITOR);
    /* Create the menu */
    QAction *actionSuggest = new QAction(tr("Give Suggestions"), this);
    QAction *actionIgnore  = new QAction(tr("Ignore Word"), this);
    QAction *actionAdd     = new QAction(tr("Add Word"), this);
    QAction *actionLucky   = new QAction(tr("Feeling Lucky"), this);
    Core::Command *cmdSuggest = Core::ActionManager::registerAction(actionSuggest, Constants::ACTION_SUGGEST_ID, textContext);
    Core::Command *cmdIgnore  = Core::ActionManager::registerAction(actionIgnore, Constants::ACTION_IGNORE_ID, textContext);
    Core::Command *cmdAdd     = Core::ActionManager::registerAction(actionAdd, Constants::ACTION_ADD_ID, textContext);
    Core::Command *cmdLucky   = Core::ActionManager::registerAction(actionLucky, Constants::ACTION_LUCKY_ID, textContext);
    cmdSuggest->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+S")));
    cmdIgnore->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+I")));
    cmdAdd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+A")));
    cmdLucky->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+L")));
    connect(actionSuggest, &QAction::triggered, m_spellCheckerCore.get (), &SpellCheckerCore::giveSuggestionsForWordUnderCursor);
    connect(actionIgnore, &QAction::triggered, m_spellCheckerCore.get (), &SpellCheckerCore::ignoreWordUnderCursor);
    connect(actionAdd, &QAction::triggered, m_spellCheckerCore.get (), &SpellCheckerCore::addWordUnderCursor);
    connect(actionLucky, &QAction::triggered, m_spellCheckerCore.get (), &SpellCheckerCore::replaceWordUnderCursorFirstSuggestion);
    connect(m_spellCheckerCore.get(), &SpellCheckerCore::wordUnderCursorMistake, actionSuggest, &QAction::setEnabled);
    connect(m_spellCheckerCore.get (), &SpellCheckerCore::wordUnderCursorMistake, actionIgnore, &QAction::setEnabled);
    connect(m_spellCheckerCore.get (), &SpellCheckerCore::wordUnderCursorMistake, actionAdd, &QAction::setEnabled);
    connect(m_spellCheckerCore.get (), &SpellCheckerCore::wordUnderCursorMistake, [=](bool isMistake, const SpellChecker::Word& word) {
        actionLucky->setEnabled(isMistake && (word.suggestions.isEmpty() == false));
    });

    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(tr("Spell Check"));
    menu->addAction(cmdSuggest);
    menu->addAction(cmdIgnore);
    menu->addAction(cmdAdd);
    menu->addAction(cmdLucky);
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    /* Action Container for the context menu on the Right Click Menu */
    Core::ActionContainer *contextMenu = Core::ActionManager::createMenu(Constants::CONTEXT_MENU_ID);
    contextMenu->menu()->setTitle(tr("Spell Check"));
    contextMenu->addAction(cmdSuggest);
    contextMenu->addAction(cmdIgnore);
    contextMenu->addAction(cmdAdd);
    contextMenu->addAction(cmdLucky);
    contextMenu->addSeparator();
    /* Add 5 dummy actions that will be used for spelling mistakes that can be fixed from the context menu */
    QVector<const char *> holderActionIds {Constants::ACTION_HOLDER1_ID, Constants::ACTION_HOLDER2_ID, Constants::ACTION_HOLDER3_ID, Constants::ACTION_HOLDER4_ID, Constants::ACTION_HOLDER5_ID};
    for(int count = 0 ; count < holderActionIds.size(); ++count) {
        QAction *actionHolder = new QAction(QStringLiteral(""), this);
        Core::Command *cmdHolder = Core::ActionManager::registerAction(actionHolder, holderActionIds[count], textContext);
        contextMenu->addAction(cmdHolder);
    }
    /* Set the right click context menu only enabled if the word under the cursor is a spelling mistake */
    connect(m_spellCheckerCore.get (), &SpellCheckerCore::wordUnderCursorMistake, contextMenu->menu(), &QMenu::setEnabled);

    /* Create the navigation widget factory */
    m_navigationWidgetFactory.reset (new NavigationWidgetFactory(m_spellCheckerCore->spellingMistakesModel()));

    /* --- Create the default Spell Checker and Document Parser --- */
    /* Hunspell Spell Checker */
    m_spellChecker.reset (new SpellChecker::Checker::Hunspell::HunspellChecker());
    m_spellCheckerCore->setSpellChecker(m_spellChecker.get ());

    /* Cpp Document Parser */
    m_cppParser.reset (new SpellChecker::CppSpellChecker::Internal::CppDocumentParser());

    m_spellCheckerCore->addDocumentParser(m_cppParser.get());

    /* Quick fix provider */
    m_spellCheckCppQuickFixFactory.reset (new SpellCheckCppQuickFixFactory);

    return true;
}

void SpellCheckerPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized function, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag SpellCheckerPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}
