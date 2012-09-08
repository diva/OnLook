TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += ../../
CONFIG -= app_bundle
CONFIG += console

QT += webkit network

mac {
    DEFINES += LL_OSX
    LIBS += $$PWD/libllqtwebkit.dylib
}

win32 {
    DEFINES += _WINDOWS
    INCLUDEPATH += ../
    DESTDIR=../build
    LIBS += user32.lib 
    release {
      LIBS += $$PWD/../../Release/llqtwebkit.lib
    }
}

include(../../static.pri)

SOURCES += textmode.cpp
