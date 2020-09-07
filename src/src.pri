#

INCLUDEPATH += $${PWD}/../

SOURCES += \
        $${PWD}/spellcheckerplugin.cpp \
        $${PWD}/spellcheckercore.cpp \
        $${PWD}/idocumentparser.cpp \
        $${PWD}/spellingmistakesmodel.cpp \
        $${PWD}/outputpane.cpp \
        $${PWD}/ISpellChecker.cpp \
        $${PWD}/spellcheckercoreoptionspage.cpp \
        $${PWD}/spellcheckercoresettings.cpp \
        $${PWD}/spellcheckercoreoptionswidget.cpp \
        $${PWD}/suggestionsdialog.cpp \
        $${PWD}/NavigationWidget.cpp \
        $${PWD}/ProjectMistakesModel.cpp \
        $${PWD}/spellcheckquickfix.cpp

HEADERS += \
        $${PWD}/spellcheckerplugin.h\
        $${PWD}/spellchecker_global.h\
        $${PWD}/spellcheckerconstants.h \
        $${PWD}/spellcheckercore.h \
        $${PWD}/idocumentparser.h \
        $${PWD}/Word.h \
        $${PWD}/spellingmistakesmodel.h \
        $${PWD}/outputpane.h \
        $${PWD}/ISpellChecker.h \
        $${PWD}/spellcheckercoreoptionspage.h \
        $${PWD}/spellcheckercoresettings.h \
        $${PWD}/spellcheckercoreoptionswidget.h \
        $${PWD}/suggestionsdialog.h \
        $${PWD}/NavigationWidget.h \
        $${PWD}/ProjectMistakesModel.h \
        $${PWD}/spellcheckquickfix.h

FORMS += \
        $${PWD}/spellcheckercoreoptionswidget.ui \
        $${PWD}/suggestionsdialog.ui


# Include a pri file with the list of parsers
include($${PWD}/Parsers/Parsers.pri)
# Include a pri file with the list of spell checkers
include($${PWD}/SpellCheckers/SpellCheckers.pri)

