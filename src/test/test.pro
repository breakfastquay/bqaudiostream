
TEMPLATE = app
TARGET = test-audiostream

QT += testlib

DESTDIR = ../../../out
QMAKE_LIBDIR += ../../../out ../../../../dataquay

INCLUDEPATH += . ../.. ../../../..
DEPENDPATH += . ../.. ../../../..

!win32-* {
    PRE_TARGETDEPS += ../../../out/libturbot.a 
}

load(../../../platform.prf)
TEMPLATE += platform

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

win32:QMAKE_POST_LINK=$${DESTDIR}$${TARGET}.exe

