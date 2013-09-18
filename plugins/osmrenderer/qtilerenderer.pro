TEMPLATE = lib
CONFIG += plugin static

PROTOS = ../../utils/osm/osmformat.proto ../../utils/osm/fileformat.proto
include(../../utils/osm/protobuf.pri)

PRE_TARGETDEPS += osmformat.pb.h fileformat.pb.h osmformat.pb.cc fileformat.pb.cc

CONFIG += link_pkgconfig
INCLUDEPATH += ../.. ../osmimporter/ qtilerenderer
PKGCONFIG += libxml-2.0

HEADERS += \
	 ../../interfaces/ipreprocessor.h \
	 ../../interfaces/iimporter.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 ../../utils/osm/xmlreader.h \
	 ../../utils/osm/ientityreader.h \
	 ../../utils/osm/pbfreader.h \
	qtilerenderer/preprocessor/qtilerenderer.h
SOURCES += \
	qtilerenderer/preprocessor/qtilerenderer.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

RESOURCES += \
	qtilerenderer/preprocessor/rendering_rules.qrc

!nogui {
	SOURCES += qtilerenderer/preprocessor/qrsettingsdialog.cpp
	HEADERS += qtilerenderer/preprocessor/qrsettingsdialog.h
	FORMS += qtilerenderer/preprocessor/qrsettingsdialog.ui
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
