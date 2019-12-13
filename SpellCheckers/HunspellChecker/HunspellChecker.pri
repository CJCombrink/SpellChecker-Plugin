SOURCES += \
        $$PWD/hunspellchecker.cpp \
        $$PWD/hunspelloptionswidget.cpp

HEADERS +=  \
        $$PWD/hunspellchecker.h \
        $$PWD/HunspellConstants.h \
        $$PWD/hunspelloptionswidget.h

FORMS += \
        $$PWD/hunspelloptionswidget.ui

win32|!isEmpty(LOCAL_HUNSPELL_LIB_DIR) {
  win32-msvc*:HUNSPELL_LIB_NAME=libhunspell
  win32-g++  :HUNSPELL_LIB_NAME=hunspell
  unix       :HUNSPELL_LIB_NAME=hunspell
  INCLUDEPATH += $${LOCAL_HUNSPELL_SRC_DIR}/
  LIBS        += -L$${LOCAL_HUNSPELL_LIB_DIR} -l$${HUNSPELL_LIB_NAME}
}
unix:isEmpty(LOCAL_HUNSPELL_LIB_DIR) {
  CONFIG += link_pkgconfig
  PKGCONFIG += hunspell
}
