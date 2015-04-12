TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../.. qtilerenderer

DESTDIR = ../../bin/plugins_client
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}
HEADERS += \
	../../interfaces/irenderer.h \
	../../utils/coordinates.h \
	../../utils/config.h \
	../../utils/qthelpers.h \
	rendererbase.h \
	brsettingsdialog.h \
	qtilerenderer/types.h \
	qtilerenderer/client/qtilerendererclient.h \
	qtilerenderer/client/tile-write.h \
	qtilerenderer/client/img_writer.h
SOURCES += \
	brsettingsdialog.cpp \
	rendererbase.cpp \
	qtilerenderer/client/qtilerendererclient.cpp \
	qtilerenderer/client/tile-write.cpp \
	qtilerenderer/client/agg2/agg_vcgen_stroke.cpp

FORMS += \
	brsettingsdialog.ui
