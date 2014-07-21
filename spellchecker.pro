DEFINES += SPELLCHECKER_LIBRARY

# SpellChecker files

SOURCES += \
        spellcheckerplugin.cpp \
        spellcheckercore.cpp \
        idocumentparser.cpp \
        spellingmistakesmodel.cpp \
        outputpane.cpp \
        spellcheckercoreoptionspage.cpp \
        spellcheckercoresettings.cpp \
        spellcheckercoreoptionswidget.cpp \
        suggestionsdialog.cpp

HEADERS += spellcheckerplugin.h\
        spellchecker_global.h\
        spellcheckerconstants.h \
        spellcheckercore.h \
        idocumentparser.h \
        Word.h \
        spellingmistakesmodel.h \
        outputpane.h \
        ISpellChecker.h \
        spellcheckercoreoptionspage.h \
        spellcheckercoresettings.h \
        spellcheckercoreoptionswidget.h \
        suggestionsdialog.h

FORMS += \
        spellcheckercoreoptionswidget.ui \
        suggestionsdialog.ui

# Include a pri file that sets the local paths to needed
# folders. It is done in this way so that the main pro
# file does not need to change depending on the environment of the
# system building the plugin.
include(spellchecker_local_paths.pri)
# Include a pri file with the list of parsers
include(Parsers/Parsers.pri)
# Include a pri file with the list of spell checkers
include(SpellCheckers/SpellCheckers.pri)

# Qt Creator linking
## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=$${LOCAL_QTCREATOR_SOURCES}

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=$${LOCAL_IDE_BUILD_TREE}
# Does not seem to need this one as it is added by Qt Creator already
include(spellchecker_dependencies.pri)

## uncomment to build plugin into user config directory
## <localappdata>/plugins/<ideversion>
##    where <localappdata> is e.g.
##    "%LOCALAPPDATA%\QtProject\qtcreator" on Windows Vista and later
##    "$XDG_DATA_HOME/data/QtProject/qtcreator" or "~/.local/share/data/QtProject/qtcreator" on Linux
##    "~/Library/Application Support/QtProject/Qt Creator" on Mac
# USE_USER_DESTDIR = yes
#
PROVIDER = CJC
include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)
