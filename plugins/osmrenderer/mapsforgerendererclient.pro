TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../.. ../../lib

DESTDIR = ../../bin/plugins_client
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}

HEADERS += \
	../../interfaces/irenderer.h \
	../../utils/coordinates.h \
	../../utils/config.h \
	../../utils/qthelpers.h \
	rendererbase.h \
	brsettingsdialog.h \
	mapsforgerenderer/client/mapsforgerendererclient.h \
	mapsforgerenderer/client/mapsforgetilewriter.h

SOURCES += \
	brsettingsdialog.cpp \
	rendererbase.cpp \
	mapsforgerenderer/client/mapsforgerendererclient.cpp \
	mapsforgerenderer/client/mapsforgetilewriter.cpp

FORMS += \
	brsettingsdialog.ui
