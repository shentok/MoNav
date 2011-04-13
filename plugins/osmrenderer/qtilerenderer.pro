TEMPLATE = lib
CONFIG += plugin static

PROTOS = ../osmimporter/osmformat.proto ../osmimporter/fileformat.proto
include(../osmimporter/protobuf.pri)

PRE_TARGETDEPS += osmformat.pb.h fileformat.pb.h osmformat.pb.cc fileformat.pb.cc

CONFIG += link_pkgconfig
INCLUDEPATH += ../.. ../osmimporter/
PKGCONFIG += libxml-2.0

HEADERS += qtilerenderer.h \
	 ../../interfaces/ipreprocessor.h \
	 ../../interfaces/iimporter.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 ../osmimporter/pbfreader.h
SOURCES += qtilerenderer.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -march=native \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

RESOURCES += \
	 rendering_rules.qrc

!nogui {
	SOURCES += qrsettingsdialog.cpp
	HEADERS += qrsettingsdialog.h
	FORMS += qrsettingsdialog.ui
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
