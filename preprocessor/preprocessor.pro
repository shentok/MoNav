# -------------------------------------------------
# Project created by QtCreator 2010-06-08T10:48:36
# -------------------------------------------------
TARGET = MoNav
TEMPLATE = app
SOURCES += main.cpp \
    preprocessingwindow.cpp \
    aboutdialog.cpp
HEADERS += preprocessingwindow.h \
    aboutdialog.h \
    interfaces/IImporter.h \
    utils/utils.h \
    utils/rng.h \
    utils/coordinates.h \
    utils/config.h \
    interfaces/IPreprocessor.h \
    interfaces/IRenderer.h \
    interfaces/ipreprocessor.h \
    interfaces/iimporter.h
DESTDIR = ../bin
TARGET = MoNavP
FORMS += preprocessingwindow.ui \
    aboutdialog.ui
RESOURCES += images.qrc
