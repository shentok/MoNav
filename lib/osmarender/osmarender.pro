TEMPLATE = lib
CONFIG += static

QT += xml

INCLUDEPATH += ../..

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}
HEADERS += \
    RenderTheme.h \
    Rule.h \
    RenderInstruction.h \
    instructions/Area.h \
    instructions/Caption.h \
    instructions/Circle.h \
    instructions/Line.h \
    RenderThemeHandler.h \
    RenderCallback.h \
    ElementMatcher.h \
    ElementAnyMatcher.h \
    ElementNodeMatcher.h \
    ElementWayMatcher.h \
    ClosedMatcher.h \
    ClosedAnyMatcher.h \
    ClosedWayMatcher.h \
    LinearWayMatcher.h \
    NegativeMatcher.h \
    NegativeRule.h \
    PositiveRule.h \
    AttributeMatcher.h \
    AnyMatcher.h \
    KeyMatcher.h \
    ValueMatcher.h \
    instructions/LineSymbol.h \
    instructions/PathText.h \
    instructions/Symbol.h

SOURCES += \
    RenderTheme.cpp \
    Rule.cpp \
    RenderInstruction.cpp \
    instructions/Area.cpp \
    instructions/Caption.cpp \
    instructions/Circle.cpp \
    instructions/Line.cpp \
    RenderThemeHandler.cpp \
    RenderCallback.cpp \
    ElementMatcher.cpp \
    ElementAnyMatcher.cpp \
    ElementNodeMatcher.cpp \
    ElementWayMatcher.cpp \
    ClosedMatcher.cpp \
    ClosedAnyMatcher.cpp \
    ClosedWayMatcher.cpp \
    LinearWayMatcher.cpp \
    NegativeMatcher.cpp \
    NegativeRule.cpp \
    PositiveRule.cpp \
    AttributeMatcher.cpp \
    AnyMatcher.cpp \
    KeyMatcher.cpp \
    ValueMatcher.cpp \
    instructions/LineSymbol.cpp \
    instructions/PathText.cpp \
    instructions/Symbol.cpp

RESOURCES += \
    osmarender.qrc
