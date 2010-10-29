TEMPLATE = lib
CONFIG += plugin static
CONFIG += link_pkgconfig
PKGCONFIG += libxml-2.0
PKGCONFIG += protobuf
HEADERS += osmimporter.h \
	 oisettingsdialog.h \
	 statickdtree.h \
	 interfaces/iimporter.h \
	 utils/coordinates.h \
	 utils/config.h \
	 bz2input.h \
	 utils/intersection.h \
	 utils/qthelpers.h \
	 xmlreader.h \
	 ientityreader.h \
	 pbfreader.h \
	 "protobuff definitions/osmformat.pb.h" \
	 "protobuff definitions/fileformat.pb.h" \
    lzma/Types.h \
    lzma/LzmaDec.h \
    waymodificatorwidget.h \
    nodemodificatorwidget.h \
    types.h \
    highwaytypewidget.h
SOURCES += osmimporter.cpp \
	 oisettingsdialog.cpp \
	 "protobuff definitions/osmformat.pb.cc" \
	 "protobuff definitions/fileformat.pb.cc" \
    lzma/LzmaDec.c \
    waymodificatorwidget.cpp \
    nodemodificatorwidget.cpp \
    highwaytypewidget.cpp
DESTDIR = ../../bin/plugins_preprocessor
FORMS += oisettingsdialog.ui \
    waymodificatorwidget.ui \
    nodemodificatorwidget.ui \
    highwaytypewidget.ui
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

RESOURCES += \
	 speedprofiles.qrc
