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
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}
HEADERS += \
	../../interfaces/irenderer.h \
	../../utils/coordinates.h \
	../../utils/config.h \
	../../utils/intersection.h \
	../../utils/qthelpers.h \
	rendererbase.h \
	brsettingsdialog.h \
	mapnikrenderer/client/mapnikrendererclient.h

SOURCES += \
	rendererbase.cpp \
	brsettingsdialog.cpp \
	mapnikrenderer/client/mapnikrendererclient.cpp

FORMS += \
	brsettingsdialog.ui
