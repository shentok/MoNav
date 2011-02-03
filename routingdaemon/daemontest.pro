TEMPLATE = app
DESTDIR = ../bin

INCLUDEPATH += ..

TARGET = daemon-test
QT -= gui
QT +=network
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -march=native \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}
SOURCES += \
	 test.cpp

HEADERS += \
	 signals.h
