#-------------------------------------------------
#
# Project created by QtCreator 2010-06-25T08:48:06
#
#-------------------------------------------------

TARGET = gpsgrid
TEMPLATE = lib
CONFIG += plugin

DESTDIR = ../../bin/plugins_preprocessor

SOURCES += gpsgrid.cpp \
    ggdialog.cpp

HEADERS += gpsgrid.h \
    interfaces/ipreprocessor.h \
    interfaces/iimporter.h \
    utils/coordinates.h \
    utils/config.h \
    ggdialog.h \
    cell.h \
    table.h

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 \
	 -march=native \
	 -Wno-unused-function
QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
	 -fopenmp

FORMS += \
    ggdialog.ui
