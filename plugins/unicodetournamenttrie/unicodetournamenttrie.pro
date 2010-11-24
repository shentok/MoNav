#-------------------------------------------------
#
# Project created by QtCreator 2010-06-22T13:51:45
#
#-------------------------------------------------

TARGET = unicodetournamenttrie
DESTDIR = ../../bin/plugins_preprocessor

INCLUDEPATH += ../..

TEMPLATE = lib
CONFIG += plugin static

SOURCES += unicodetournamenttrie.cpp \
	 uttsettingsdialog.cpp

HEADERS += unicodetournamenttrie.h \
	 utils/coordinates.h \
	 utils/config.h \
	 interfaces/iimporter.h \
	 interfaces/ipreprocessor.h \
	 trie.h \
	 uttsettingsdialog.h \
	 utils/bithelpers.h \
	 utils/qthelpers.h \
	 utils/edgeconnector.h

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

FORMS += \
	 uttsettingsdialog.ui
