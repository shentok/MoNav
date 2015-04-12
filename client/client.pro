# -------------------------------------------------
# Project created by QtCreator 2010-06-15T15:30:10
# -------------------------------------------------

TARGET = MoNavClient
TEMPLATE = app

INCLUDEPATH += ..

SOURCES += main.cpp \
	paintwidget.cpp \
	addressdialog.cpp \
	bookmarksdialog.cpp \
	routedescriptiondialog.cpp \
	mapdata.cpp \
	routinglogic.cpp \
	overlaywidget.cpp \
	scrollarea.cpp \
	gpsdialog.cpp \
	generalsettingsdialog.cpp \
	logger.cpp \
    ../utils/directoryunpacker.cpp \
    ../utils/lzma/LzmaDec.c \
    mappackageswidget.cpp \
    mainwindow.cpp \
    mapmoduleswidget.cpp \
    placechooser.cpp \
    globalsettings.cpp \
    streetchooser.cpp \
    worldmapchooser.cpp

HEADERS += \
	paintwidget.h \
	../utils/coordinates.h \
	../utils/config.h \
	../interfaces/irenderer.h \
	../interfaces/iaddresslookup.h \
	addressdialog.h \
	../interfaces/igpslookup.h \
	../interfaces/irouter.h \
	bookmarksdialog.h \
	routedescriptiondialog.h \
	descriptiongenerator.h \
	mapdata.h \
	routinglogic.h \
	fullscreenexitbutton.h \
	overlaywidget.h \
	scrollarea.h \
	gpsdialog.h \
	generalsettingsdialog.h \
	logger.h \
    ../utils/directoryunpacker.h \
    ../utils/lzma/LzmaDec.h \
    mappackageswidget.h \
    mainwindow.h \
    mapmoduleswidget.h \
    placechooser.h \
    globalsettings.h \
    streetchooser.h \
    worldmapchooser.h

FORMS += \
	paintwidget.ui \
	addressdialog.ui \
	bookmarksdialog.ui \
	routedescriptiondialog.ui \
	gpsdialog.ui \
	generalsettingsdialog.ui \
    mappackageswidget.ui \
    mainwindow.ui \
    mapmoduleswidget.ui \
    placechooser.ui \
    streetchooser.ui \
    worldmapchooser.ui

DESTDIR = ../bin

TARGET = monav
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		-Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}
maemo5 {
	QT += maemo5
}
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets concurrent positioning
}

RESOURCES += images.qrc
RC_FILE = ../images/WindowsResources.rc

LIBS += -L../bin/plugins_client -lmapnikrendererclient -lmapsforgerendererclient -lcontractionhierarchiesclient -lgpsgridclient -losmrendererclient -lunicodetournamenttrieclient -lqtilerendererclient -L../lib/mapsforgerenderer -lmapsforgerenderer -L../lib/mapsforgereader -lmapsforgereader -L../lib/osmarender -losmarender

# Required by osmrendererclient
QT += network xml
CONFIG += mobility
MOBILITY += location
# Required to get a non-debug build (at least on Windows)
# CONFIG += release

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/bin
    } else {
        target.path = /usr/local/bin
    }
    INSTALLS += target
}

DISTFILES += \
	../platform/android/AndroidManifest.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../platform/android
