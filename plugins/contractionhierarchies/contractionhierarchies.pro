TEMPLATE = lib
CONFIG += plugin
DESTDIR = ../../bin/plugins_preprocessor
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 \
    -march=native \
    -Wno-unused-function \
    -fopenmp
QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
    -fopenmp
LIBS += -fopenmp \
    -lmapnik
HEADERS += contractionhierarchies.h \
	 dynamicgraph.h \
	 contractioncleanup.h \
	 compressedchgraph.h \
	 blockcache.h \
	 binaryheap.h \
	 contractor.h \
    interfaces/ipreprocessor.h \
    utils/utils.h \
    utils/coordinates.h \
    utils/config.h \
    chsettingsdialog.h
SOURCES += contractionhierarchies.cpp \
    chsettingsdialog.cpp
FORMS += chsettingsdialog.ui
