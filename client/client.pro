# -------------------------------------------------
# Project created by QtCreator 2010-06-15T15:30:10
# -------------------------------------------------
TARGET = MoNavClient
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    mapview.cpp \
    paintwidget.cpp \
    addressdialog.cpp \
    bookmarksdialog.cpp
HEADERS += mainwindow.h \
    interfaces/IRenderer.h \
    mapview.h \
    paintwidget.h \
    utils/coordinates.h \
    utils/config.h \
    interfaces/irenderer.h \
    interfaces/iaddresslookup.h \
    addressdialog.h \
    interfaces/igpslookup.h \
    interfaces/irouter.h \
    bookmarksdialog.h
FORMS += mainwindow.ui \
    mapview.ui \
    paintwidget.ui \
    addressdialog.ui \
    bookmarksdialog.ui
DESTDIR = ../bin
TARGET = MoNavC
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 \
    -march=native \
	 -Wno-unused-function
QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
RESOURCES += images.qrc
QT += opengl
