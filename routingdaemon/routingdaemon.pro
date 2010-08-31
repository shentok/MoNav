TEMPLATE = app
DESTDIR = ../bin
TARGET = MoNavD
QT -= gui
QT +=network

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -march=native \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

LIBS += -L../bin/plugins_client -lcontractionhierarchiesclient -lgpsgridclient

SOURCES += \
    main.cpp

HEADERS += \
    signals.h \
    routinddaemon.h

include(qtservice-2.6_1-opensource/src/qtservice.pri)
