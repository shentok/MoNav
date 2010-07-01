TEMPLATE = lib
CONFIG += plugin
DESTDIR = ../../bin/plugins_client
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 \
    -march=native \
    -Wno-unused-function \
    -fopenmp
QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
    -fopenmp
LIBS += -fopenmp

HEADERS += \
    utils/utils.h \
    utils/coordinates.h \
    utils/config.h \
    blockcache.h \
    binaryheap.h \
    interfaces/irouter.h \
    contractionhierarchiesclient.h

SOURCES += \
    contractionhierarchiesclient.cpp
