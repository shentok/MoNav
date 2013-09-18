TEMPLATE = lib
CONFIG += static

INCLUDEPATH += ../..

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

HEADERS += \
    header/MapFileHeader.h \
    header/MapFileInfo.h \
    header/OptionalFields.h \
    header/SubFileParameter.h \
    BoundingBox.h \
    IndexCache.h \
    LatLong.h \
    MapDatabase.h \
    MercatorProjection.h \
    PointOfInterest.h \
    QueryParameters.h \
    ReadBuffer.h \
    Tag.h \
    TileId.h \
    VectorTile.h \
    Way.h

SOURCES += \
    header/MapFileHeader.cpp \
    header/MapFileInfo.cpp \
    header/OptionalFields.cpp \
    header/SubFileParameter.cpp \
    BoundingBox.cpp \
    IndexCache.cpp \
    LatLong.cpp \
    MapDatabase.cpp \
    MercatorProjection.cpp \
    PointOfInterest.cpp \
    QueryParameters.cpp \
    ReadBuffer.cpp \
    Tag.cpp \
    TileId.cpp \
    VectorTile.cpp \
    Way.cpp
