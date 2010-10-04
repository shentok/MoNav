# -------------------------------------------------
# Project created by QtCreator 2010-06-08T10:48:36
# -------------------------------------------------
TEMPLATE = app
CONFIG += link_pkgconfig
PKGCONFIG += libxml-2.0
PKGCONFIG += protobuf
SOURCES += main.cpp \
	 preprocessingwindow.cpp \
	 aboutdialog.cpp
HEADERS += preprocessingwindow.h \
	 aboutdialog.h \
	 interfaces/iimporter.h \
	 utils/coordinates.h \
	 utils/config.h \
	 interfaces/ipreprocessor.h \
	 interfaces/irenderer.h \
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
LIBS += -fopenmp -lmapnik -lbz2
LIBS += -L../bin/plugins_preprocessor -lmapnikrenderer -lcontractionhierarchies -lgpsgrid -losmrenderer -lunicodetournamenttrie -losmimporter
