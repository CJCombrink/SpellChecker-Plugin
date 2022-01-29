# to configure run something like
# cmake "-DCMAKE_PREFIX_PATH=<path_to_qt>;<path_to_qtcreator>;<path_to_hunspell>" -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja

cmake_minimum_required(VERSION 3.16)

## Add paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(SpellChecker)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
set(QtX Qt${QT_VERSION_MAJOR})

add_qtc_plugin(SpellChecker
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::TextEditor
    QtCreator::ProjectExplorer
    QtCreator::CppEditor
  DEPENDS
    ${QtX}::Widgets
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    .github/workflows/build.yaml
    COPYING
    COPYING.LESSER
    LICENSE
    README.md
    uncrustify.cfg
)

extend_qtc_plugin(SpellChecker
  SOURCES_PREFIX src
  SOURCES
    ISpellChecker.cpp
    ISpellChecker.h
    NavigationWidget.cpp
    NavigationWidget.h
    ProjectMistakesModel.cpp
    ProjectMistakesModel.h
    Word.h
    idocumentparser.cpp
    idocumentparser.h
    outputpane.cpp
    outputpane.h
    spellchecker_global.h
    spellcheckerconstants.h
    spellcheckercore.cpp
    spellcheckercore.h
    spellcheckercoreoptionspage.cpp
    spellcheckercoreoptionspage.h
    spellcheckercoreoptionswidget.cpp
    spellcheckercoreoptionswidget.h
    spellcheckercoreoptionswidget.ui
    spellcheckercoresettings.cpp
    spellcheckercoresettings.h
    spellcheckerplugin.cpp
    spellcheckerplugin.h
    spellcheckquickfix.cpp
    spellcheckquickfix.h
    spellingmistakesmodel.cpp
    spellingmistakesmodel.h
    suggestionsdialog.cpp
    suggestionsdialog.h
    suggestionsdialog.ui
)

extend_qtc_plugin(SpellChecker
  SOURCES_PREFIX src/Parsers/CppParser
  SOURCES
     cppdocumentparser.cpp
     cppdocumentparser.h
     cppdocumentprocessor.cpp
     cppdocumentprocessor.h
     cppparserconstants.h
     cppparseroptionspage.cpp
     cppparseroptionspage.h
     cppparseroptionswidget.cpp
     cppparseroptionswidget.h
     cppparseroptionswidget.ui
     cppparsersettings.cpp
     cppparsersettings.h
)

find_package(hunspell)
extend_qtc_plugin(SpellChecker
  CONDITION TARGET hunspell::hunspell
  DEPENDS hunspell::hunspell
  SOURCES_PREFIX src/SpellCheckers/HunspellChecker/
  SOURCES
    HunspellConstants.h
    hunspellchecker.cpp
    hunspellchecker.h
    hunspelloptionswidget.cpp
    hunspelloptionswidget.h
    hunspelloptionswidget.ui
)
