
TEMPLATE = app
TARGET = test-audiostream
win*: TARGET = "TestAudiostream"

QT += testlib

DESTDIR = ../../../out
QMAKE_LIBDIR += ../../../out ../../../../dataquay

INCLUDEPATH += . ../.. ../../../..
DEPENDPATH += . ../.. ../../../..

!win32-* {
    PRE_TARGETDEPS += ../../../out/libturbot.a 
}

include(../../../platform.pri)

OBJECTS_DIR = o
MOC_DIR = o

HEADERS += AudioStreamTestData.h TestAudioStreamRead.h TestSimpleWavRead.h TestAudioStreamColumnReader.h TestWavReadWrite.h
SOURCES += main.cpp

!win32 {
    !macx* {
        QMAKE_POST_LINK=$${DESTDIR}/$${TARGET}
    }
    macx* {
        QMAKE_POST_LINK=$${DESTDIR}/$${TARGET}.app/Contents/MacOS/$${TARGET}
    }
}

win32-g++:QMAKE_POST_LINK=$${DESTDIR}$${TARGET}.exe

