TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += ../../
CONFIG -= app_bundle

QT += webkit opengl network

!mac {
unix {
    DEFINES += LL_LINUX
    LIBS += -lglui -lglut
    LIBS += $$PWD/../../libllqtwebkit.a
}
}

mac {
    DEFINES += LL_OSX
    LIBS += -framework GLUT -framework OpenGL
    LIBS += $$PWD/libllqtwebkit.dylib
}

win32 {
    DEFINES += _WINDOWS
    INCLUDEPATH += ../
    INCLUDEPATH += $$PWD/../../stage/packages/include
    DESTDIR=../build
    release {
        LIBS += $$PWD/../../Release/llqtwebkit.lib
        LIBS += $$PWD/../build/freeglut_static.lib
        LIBS += comdlg32.lib
    }
}

include(../../static.pri)

SOURCES += 3dgl.cpp zpr.c
