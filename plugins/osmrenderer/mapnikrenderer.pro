TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

CONFIG += link_pkgconfig
PKGCONFIG += freetype2
HEADERS += \
	 ../../interfaces/ipreprocessor.h \
	 ../../interfaces/iimporter.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 ../../utils/qthelpers.h \
	mapnikrenderer/preprocessor/mapnikrenderer.h
SOURCES += \
	mapnikrenderer/preprocessor/mapnikrenderer.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function \
		 -fopenmp
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
		 -fopenmp
}
LIBS += -fopenmp \
	 -lmapnik

!nogui {
	FORMS += mapnikrenderer/preprocessor/mrsettingsdialog.ui
	SOURCES += mapnikrenderer/preprocessor/mrsettingsdialog.cpp
	HEADERS += mapnikrenderer/preprocessor/mrsettingsdialog.h
    greaterThan(QT_MAJOR_VERSION, 4) {
        QT += widgets
    }
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
