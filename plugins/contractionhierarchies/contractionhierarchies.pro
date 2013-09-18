TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function \
		 -fopenmp
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
		 -fopenmp
}
LIBS += -fopenmp
HEADERS += \
	../../interfaces/ipreprocessor.h \
	../../interfaces/irouter.h \
	../../utils/coordinates.h \
	../../utils/config.h \
	../../utils/bithelpers.h \
	../../utils/qthelpers.h \
	contractionhierarchies.h \
	dynamicgraph.h \
	contractioncleanup.h \
	blockcache.h \
	binaryheap.h \
	contractor.h \
	compressedgraph.h \
	compressedgraphbuilder.h

SOURCES += contractionhierarchies.cpp

!nogui {
	SOURCES += chsettingsdialog.cpp
	FORMS += chsettingsdialog.ui
	HEADERS += chsettingsdialog.h
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
