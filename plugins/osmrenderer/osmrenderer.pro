TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

HEADERS += \
	 ../../interfaces/ipreprocessor.h \
	 ../../interfaces/iimporter.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	osmrenderer/preprocessor/osmrenderer.h
SOURCES += \
	osmrenderer/preprocessor/osmrenderer.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

!nogui {
	SOURCES += osmrenderer/preprocessor/orsettingsdialog.cpp
	HEADERS += osmrenderer/preprocessor/orsettingsdialog.h
	FORMS += osmrenderer/preprocessor/orsettingsdialog.ui
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
