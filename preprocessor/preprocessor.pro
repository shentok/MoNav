# -------------------------------------------------
# Project created by QtCreator 2010-06-08T10:48:36
# -------------------------------------------------
TEMPLATE = app
SOURCES += main.cpp \
    preprocessingwindow.cpp \
    aboutdialog.cpp
HEADERS += preprocessingwindow.h \
    aboutdialog.h \
    interfaces/IImporter.h \
    utils/utils.h \
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
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -march=native \
		 -Wno-unused-function \
		 -fopenmp
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
		 -fopenmp
}
LIBS += -L../bin/plugins_preprocessor -lmapnikrenderer -lcontractionhierarchies -lgpsgrid -losmrenderer -lunicodetournamenttrie -losmimporter
LIBS += -fopenmp -lmapnik -lbz2 -lxml2
