/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "TestSimpleWavRead.h"
#include "TestAudioStreamColumnReader.h"
#include <QtTest>

#include <iostream>

int main(int argc, char *argv[])
{
    int good = 0, bad = 0;

    QCoreApplication app(argc, argv);

    {
	Turbot::TestSimpleWavRead t;
	if (QTest::qExec(&t, argc, argv) == 0) ++good;
	else ++bad;
    }

    {
	Turbot::TestAudioStreamColumnReader t;
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

