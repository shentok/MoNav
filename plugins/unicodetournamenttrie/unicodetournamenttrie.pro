#-------------------------------------------------
#
# Project created by QtCreator 2010-06-22T13:51:45
#
#-------------------------------------------------

TARGET = unicodetournamenttrie
DESTDIR = ../../bin/plugins_preprocessor
TEMPLATE = lib
CONFIG += plugin static

SOURCES += unicodetournamenttrie.cpp

HEADERS += unicodetournamenttrie.h \
    utils/coordinates.h \
    utils/config.h \
    interfaces/iimporter.h \
    interfaces/ipreprocessor.h \
    trie.h \
    utils/utils.h

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -Wno-unused-function
QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
