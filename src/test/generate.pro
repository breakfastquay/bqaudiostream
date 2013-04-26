
TEMPLATE = app
TARGET = audiostream-test-generate

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

HEADERS += AudioStreamTestData.h 
SOURCES += generate.cpp

