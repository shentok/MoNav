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
	renderer/ShapeContainer.h \
	renderer/PointTextContainer.h \
	renderer/SymbolContainer.h \
	renderer/ShapePaintContainer.h \
	renderer/WayTextContainer.h \
	renderer/PolylineContainer.h \
	renderer/CircleContainer.h \
	util/TileFactory.h \
	DatabaseRenderer.h \
	TileRasterer.h

SOURCES += \
	renderer/PointTextContainer.cpp \
	renderer/SymbolContainer.cpp \
	renderer/ShapePaintContainer.cpp \
	renderer/WayTextContainer.cpp \
	renderer/PolylineContainer.cpp \
	renderer/CircleContainer.cpp \
	util/TileFactory.cpp \
	DatabaseRenderer.cpp \
	TileRasterer.cpp
