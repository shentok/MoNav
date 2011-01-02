TEMPLATE = lib
CONFIG += plugin static

CONFIG += link_pkgconfig
INCLUDEPATH += ../.. ../osmimporter/
PKGCONFIG += libxml-2.0

HEADERS += qtilerenderer.h \
	 interfaces/ipreprocessor.h \
	 interfaces/iimporter.h \
	 utils/coordinates.h \
	 utils/config.h \
	 qrsettingsdialog.h
SOURCES += qtilerenderer.cpp \
	 qrsettingsdialog.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -march=native \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

FORMS += \
	 qrsettingsdialog.ui
