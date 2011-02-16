TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

HEADERS += \
    testimporter.h
SOURCES += \
    testimporter.cpp
DESTDIR = ../../bin/plugins_preprocessor
FORMS +=
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}
