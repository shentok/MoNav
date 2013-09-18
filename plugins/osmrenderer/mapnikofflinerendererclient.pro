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
	../../utils/qthelpers.h \
	rendererbase.h \
	brsettingsdialog.h \
	mapnikofflinerenderer/client/mapnikofflinerendererclient.h \
	mapnikofflinerenderer/client/mapniktilewrite.h

SOURCES += \
	brsettingsdialog.cpp \
	rendererbase.cpp \
	mapnikofflinerenderer/client/mapnikofflinerendererclient.cpp \
	mapnikofflinerenderer/client/mapniktilewrite.cpp

FORMS += \
	brsettingsdialog.ui

LIBS += -lmapnik
