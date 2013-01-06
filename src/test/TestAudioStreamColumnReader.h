/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef TEST_AUDIO_STREAM_COLUMN_READER_H
#define TEST_AUDIO_STREAM_COLUMN_READER_H

#include <QObject>
#include <QtTest>

#include "AudioStreamColumnReader.h"

#include "taf/AudioImporter.h"
#include "taf/TurbotAudioFileReader.h"

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
	// TAF file. They should have identical data.

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
	
	for (int i = 0; i < colReader.getWidth(); ++i) {
	    for (int c = 0; c < colReader.getChannelCount(); ++c) {
		bool ar = colReader.getColumnPolarInterleaved(i, c, a);
		bool br = tafReader.getColumnPolarInterleaved(i, c, b);
		QCOMPARE(ar, br);
                for (int j = 0; j < sz; ++j) {
                    // The 1e-5 in the next line is too forgiving, really
                    QVERIFY2(fabs(a[j] - b[j]) < 1e-5,
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

	tafReader.close();

	}

	QFile(tafname).remove();
	colReader.close();
    }


};

}

#endif
