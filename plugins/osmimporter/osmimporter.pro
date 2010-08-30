TEMPLATE = lib
CONFIG += plugin static
CONFIG += link_pkgconfig
PKGCONFIG += libxml-2.0
HEADERS += osmimporter.h \
	 oisettingsdialog.h \
	 statickdtree.h \
    interfaces/iimporter.h \
    utils/coordinates.h \
    utils/config.h \
    bz2input.h
SOURCES += osmimporter.cpp \
	 oisettingsdialog.cpp
DESTDIR = ../../bin/plugins_preprocessor
FORMS += oisettingsdialog.ui
INCLUDEPATH += /usr/include/libxml2
LIBS += -lxml2
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

RESOURCES += \
    speedprofiles.qrc
