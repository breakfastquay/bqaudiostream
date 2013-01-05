/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef TEST_AUDIO_STREAM_COLUMN_READER_H
#define TEST_AUDIO_STREAM_COLUMN_READER_H

#include <QObject>
#include <QtTest>

#include "AudioStreamColumnReader.h"

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


};

}

#endif
