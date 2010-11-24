TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

HEADERS += osmrenderer.h \
	 interfaces/ipreprocessor.h \
	 interfaces/iimporter.h \
	 utils/coordinates.h \
	 utils/config.h \
	 orsettingsdialog.h
SOURCES += osmrenderer.cpp \
	 orsettingsdialog.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -march=native \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

FORMS += \
	 orsettingsdialog.ui
