TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

DESTDIR = ../../bin/plugins_client
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}
HEADERS += \
	 ../../interfaces/irenderer.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 ../../utils/intersection.h \
	rendererbase.h \
	brsettingsdialog.h \
	osmrenderer/client/osmrsettingsdialog.h \
	osmrenderer/client/osmrendererclient.h
SOURCES += \
	rendererbase.cpp \
	brsettingsdialog.cpp \
	osmrenderer/client/osmrsettingsdialog.cpp \
	osmrenderer/client/osmrendererclient.cpp

QT += network

FORMS += \
	brsettingsdialog.ui \
	osmrenderer/client/osmrsettingsdialog.ui
