/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef TEST_AUDIO_STREAM_COLUMN_READER_H
#define TEST_AUDIO_STREAM_COLUMN_READER_H

#include <QObject>
#include <QtTest>

#include "AudioStreamColumnReader.h"

#include "taf/AudioImporter.h"
#include "taf/TurbotAudioFileReader.h"
#include "taf/TurbotAudioMetadataReader.h"

#include <iostream>

using std::cerr;
using std::endl;

namespace Turbot {

class TestAudioStreamColumnReader : public QObject
{
    Q_OBJECT

    // This is a 44.1KHz 16-bit mono WAV file with 20 samples in it,
    // with a 1 at the start, -1 at the end and 0 elsewhere
    static const char *shorttest() { 
	static const char *f = "../../../resources/testfiles/20samples.wav";
	return f;
    }

    // This one is a 44.1KHz, 32-bit float WAV, about 13 seconds of music
    static const char *longertest() { 
	static const char *f = "../../../resources/testfiles/clip.wav";
	return f;
    }

private slots:
    void create() {
	AudioStreamColumnReader reader(shorttest());
	QCOMPARE(reader.getFileName(), QString(shorttest()));
    }

    void open() {
	AudioStreamColumnReader reader(shorttest());
	reader.open();
	QCOMPARE(reader.getChannelCount(), 1);
	reader.close();
    }

    void width() {
	AudioStreamColumnReader reader(shorttest());
	reader.open();
	// The short file contains only one block of data, but because
	// of overlaps, from hop 256 and blocksize 1024 we expect to
	// get back columns starting at -768, -512, -256, and 0
	QCOMPARE(reader.getWidth(), 4);
	reader.close();
    }

    void compare() {
	// Load WAV file using the column reader, and also import to a
	// TAF file. They should have identical data and metadata.

	AudioStreamColumnReader colReader(longertest());
	colReader.open();

	AudioImporter importer;
	Timebase timebase(44100, 1024, 256);
	QString tafname = importer.importAudioFile
	    (longertest(), QDir("."), timebase);

	{

	TurbotAudioFileReader tafReader(tafname);
	tafReader.open();

	QCOMPARE(colReader.getTimebase(), tafReader.getTimebase());
	QCOMPARE(colReader.getChannelCount(), tafReader.getChannelCount());
	QCOMPARE(colReader.getWidth(), tafReader.getWidth());
	QCOMPARE(colReader.getHeight(), tafReader.getHeight());

	int sz = colReader.getHeight()*2;
	turbot_sample_t *a = new turbot_sample_t[sz];
	turbot_sample_t *b = new turbot_sample_t[sz];
	
        double tolerance = 1e-5;

	for (int i = 0; i < colReader.getWidth(); ++i) {
	    for (int c = 0; c < colReader.getChannelCount(); ++c) {
		bool ar = colReader.getColumnPolarInterleaved(i, c, a);
		bool br = tafReader.getColumnPolarInterleaved(i, c, b);
		QCOMPARE(ar, br);
                for (int j = 0; j < sz; ++j) {
                    // Do the test first, then call QVERIFY2 only if
                    // it fails -- so as to avoid constructing the
                    // string every time
                    if (!(fabs(a[j] - b[j]) < tolerance)) {
                        QVERIFY2(fabs(a[j] - b[j]) < tolerance,
                                 QString("At column %1 channel %2 with i = %3 of %4, col reader has %5, taf reader has %6, diff = %7")
                                 .arg(i)
                                 .arg(c)
                                 .arg(j)
                                 .arg(sz)
                                 .arg(a[j])
                                 .arg(b[j])
                                 .arg(a[j] - b[j])
                                 .toLocal8Bit().data());

                    }
                }
	    }
	}

        delete[] a;
        delete[] b;

	TurbotAudioMetadataReader tamReader(tafname);
	tamReader.open();

	QCOMPARE(colReader.getTimebase(), tamReader.getTimebase());
	QCOMPARE(colReader.getChannelCount(), tamReader.getChannelCount());
	QCOMPARE(colReader.getWidth(), tamReader.getWidth());
	QCOMPARE(colReader.getHeight(), tamReader.getHeight());

        double v, w; 
        double conf1, conf2;
        bool b1, b2;

	for (int i = 0; i < colReader.getWidth(); ++i) {

            v = colReader.getAudioCurveValue(i);
            w = tamReader.getAudioCurveValue(i);
            if (fabs(v - w) >= tolerance) {
                cerr << "At column " << i << ", " << v << " - " << w << " >= " << tolerance << " for getAudioCurveValue" << endl;
                QCOMPARE(v, w);
            }

            for (int channel = 0; channel < colReader.getChannelCount(); ++channel) {
                v = colReader.getColumnUniquePower(i, channel);
                w = tamReader.getColumnUniquePower(i, channel);
                if (fabs(v - w) >= tolerance) {
                    cerr << "At column " << i << " channel " << channel << ", " << v << " - " << w << " >= " << tolerance << " for getColumnUniquePower" << endl;
                    QCOMPARE(v, w);
                }

                v = colReader.getColumnTotalPower(i, channel);
                w = tamReader.getColumnTotalPower(i, channel);
                if (fabs(v - w) >= tolerance) {
                    cerr << "At column " << i << " channel " << channel << ", " << v << " - " << w << " >= " << tolerance << " for getColumnTotalPower" << endl;
                    QCOMPARE(v, w);
                }
            }

            v = colReader.getPitchValue(i, conf1);
            w = tamReader.getPitchValue(i, conf2);
            if (fabs(v - w) >= tolerance) {
                cerr << "At column " << i << ", " << v << " - " << w << " >= " << tolerance << " for getPitchValue" << endl;
                QCOMPARE(v, w);
            }
            if (fabs(conf1 - conf2) >= tolerance) {
                cerr << "At column " << i << ", " << conf1 << " - " << conf2 << " >= " << tolerance << " for getPitchValue confidence" << endl;
                QCOMPARE(conf1, conf2);
            }

            b1 = colReader.getPhaseSync(i);
            b2 = tamReader.getPhaseSync(i);
            if (b1 != b2) {
                cerr << "At column " << i << ", " << b1 << " != " << b2 << " for getPhaseSync" << endl;
                QCOMPARE(b1, b2);
            }
	}

	tamReader.close();
        tafReader.close();

	}

	QFile(tafname).remove();
	colReader.close();
    }


};

}

#endif
