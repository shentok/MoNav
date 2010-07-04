#-------------------------------------------------
#
# Project created by QtCreator 2010-06-22T13:51:45
#
#-------------------------------------------------

DESTDIR = ../../bin/plugins_client
TEMPLATE = lib
CONFIG += plugin

HEADERS += utils/coordinates.h \
    utils/config.h \
    interfaces/iaddresslookup.h \
    trie.h \
    utils/utils.h \
    unicodetournamenttrieclient.h

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

SOURCES += \
    unicodetournamenttrieclient.cpp
