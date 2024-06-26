/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "TestSimpleWavRead.h"
#include "TestWavSeek.h"
#include "TestAudioStreamRead.h"
#include "TestWavReadWrite.h"
#include "TestWavReadWhileWriting.h"
#include <QtTest>

#include <iostream>

int main(int argc, char *argv[])
{
    int good = 0, bad = 0;

    QCoreApplication app(argc, argv);
    app.setOrganizationName("Particular Programs");
    app.setApplicationName("test-audiostream");

    {
	breakfastquay::TestSimpleWavRead t;
	if (QTest::qExec(&t, argc, argv) == 0) ++good;
	else ++bad;
    }

    {
	breakfastquay::TestWavSeek t;
	if (QTest::qExec(&t, argc, argv) == 0) ++good;
	else ++bad;
    }

    {
	breakfastquay::TestAudioStreamRead t;
	if (QTest::qExec(&t, argc, argv) == 0) ++good;
	else ++bad;
    }

    {
	breakfastquay::TestWavReadWrite t;
	if (QTest::qExec(&t, argc, argv) == 0) ++good;
	else ++bad;
    }

    {
	breakfastquay::TestWavReadWhileWriting t;
	if (QTest::qExec(&t, argc, argv) == 0) ++good;
	else ++bad;
    }

    if (bad > 0) {
	std::cerr << "\n********* " << bad << " test suite(s) failed!\n" << std::endl;
	return 1;
    } else {
        std::cerr << "All tests passed" << std::endl;
        return 0;
    }
}

