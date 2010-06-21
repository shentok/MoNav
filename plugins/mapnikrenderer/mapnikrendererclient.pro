TEMPLATE = lib
CONFIG += plugin
DESTDIR = ../../bin/plugins_client
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 \
    -march=native \
    -Wno-unused-function
QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
HEADERS += mapnikrendererclient.h \
    interfaces/irenderer.h \
    utils/coordinates.h \
    utils/config.h
SOURCES += mapnikrendererclient.cpp
