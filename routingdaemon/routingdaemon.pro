TEMPLATE = app
DESTDIR = ../bin

INCLUDEPATH += ..

DEFINES+=_7ZIP_ST

TARGET = monav-daemon
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
	 main.cpp \
	 ../utils/lzma/LzmaDec.c \
	 ../utils/directoryunpacker.cpp

HEADERS += \
	 signals.h \
	 routingdaemon.h \
	 ../utils/lzma/LzmaDec.h \
	 ../utils/directoryunpacker.h

include(qtservice-2.6_1-opensource/src/qtservice.pri)
