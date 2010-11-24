TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

CONFIG += link_pkgconfig
PKGCONFIG += freetype2
HEADERS += mapnikrenderer.h \
	 mrsettingsdialog.h \
	 interfaces/ipreprocessor.h \
	 interfaces/iimporter.h \
	 utils/coordinates.h \
	 utils/config.h \
	 utils/qthelpers.h
SOURCES += mapnikrenderer.cpp \
	 mrsettingsdialog.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -march=native \
		 -Wno-unused-function \
		 -fopenmp
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
		 -fopenmp
}
FORMS += mrsettingsdialog.ui
LIBS += -fopenmp \
	 -lmapnik
