TEMPLATE = lib
CONFIG += static

INCLUDEPATH += ../.. ..

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

HEADERS += \
	util/TileFactory.h

SOURCES += \
	util/TileFactory.cpp
