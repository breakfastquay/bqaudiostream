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

    // This is an 8KHz 8-bit mono WAV file 6 seconds long, with a
    // Dirac impulse every half a second
    static const char *bpmtest() { 
	static const char *f = "../../../resources/testfiles/120bpm.wav";
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

        int initialMiscValues = 10;
        int *ix = new int[colReader.getWidth() + initialMiscValues];

        // First we'll try some misc "random access" lookups from
        // within the file, to make sure it can do seeks (however
        // slowly)

        for (int i = 0; i < initialMiscValues/2; ++i) {
            ix[i*2] = int(colReader.getWidth() * (1 - float(i) / initialMiscValues)) - 1;
            ix[i*2+1] = ix[i*2] + 2;
        }

        // Then we'll exhaustively compare all columns

        for (int i = 0; i < colReader.getWidth(); ++i) {
            ix[initialMiscValues + i] = i;
        }

	for (int i = 0; i < colReader.getWidth() + initialMiscValues; ++i) {
            int index = ix[i];
//            std::cerr << "testing " << index << " of " << colReader.getWidth() << std::endl;
	    for (int c = 0; c < colReader.getChannelCount(); ++c) {
		bool ar = colReader.getColumnPolarInterleaved(index, c, a);
		bool br = tafReader.getColumnPolarInterleaved(index, c, b);
		QCOMPARE(ar, br);
                for (int j = 0; j < sz; ++j) {
                    // Do the test first, then call QVERIFY2 only if
                    // it fails -- so as to avoid constructing the
                    // string every time
                    if (!(fabs(a[j] - b[j]) < tolerance)) {
                        QVERIFY2(fabs(a[j] - b[j]) < tolerance,
                                 QString("At column %1 channel %2 with i = %3 of %4, col reader has %5, taf reader has %6, diff = %7")
                                 .arg(index)
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

        delete[] ix;

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

        // Now compare all those metadata values that are supposed to
        // be identical across different column readers
        
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

            // No -- we removed the pitch implementation from
            // AudioStreamColumnReader -- too expensive to
            // compute. We'd have to make it optional if restored
/*
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
*/
            // We can't compare the phase sync values, as the two use
            // different methods (AudioStreamColumnReader is causal)
	}

	tamReader.close();
        tafReader.close();

	}

	QFile(tafname).remove();
	colReader.close();
    }

    void syncpoints() {

	AudioStreamColumnReader colReader(bpmtest());
	colReader.open();

        // This file is supposed to have an impulse every 0.5 sec,
        // which at 8KHz and 6 seconds means 12 ticks every 4000
        // samples

        int hop = colReader.getTimebase().getHop();

        int tick = 0;

        for (int i = 0; i < colReader.getWidth(); ++i) {

            bool sync = colReader.getPhaseSync(i);

            if (i == int(float(tick * 4000) / hop + 0.7) && tick < 12) {
                QVERIFY2(sync == true, QString("Column %1, tick %2").arg(i).arg(tick).toLocal8Bit().data());
                ++tick;
            } else {
                QVERIFY2(sync == false, QString("Column %1, tick %2").arg(i).arg(tick).toLocal8Bit().data());
            }
        }

        colReader.close();
    }

};

}

#endif
