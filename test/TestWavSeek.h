/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef TEST_WAV_SEEK_H
#define TEST_WAV_SEEK_H

#include <QObject>
#include <QtTest>

#include "bqaudiostream/AudioReadStreamFactory.h"
#include "bqaudiostream/AudioReadStream.h"

namespace breakfastquay {

class TestWavSeek : public QObject
{
    Q_OBJECT

    // This is a 44.1KHz 16-bit mono WAV file with 20 samples in it,
    // with a 1 at the start, -1 at the end and 0 elsewhere
    static const char *testsound() { 
	static const char *f = "testfiles/20samples.wav";
	return f;
    }

private slots:
    
    void seekable() {
	AudioReadStream *s = AudioReadStreamFactory::createReadStream(testsound());
	QVERIFY(s);
	QCOMPARE(s->isSeekable(), true);
	delete s;
    }
    
    void resamplingNotSeekable() {
	AudioReadStream *s = AudioReadStreamFactory::createReadStream(testsound());
	QVERIFY(s);
	s->setRetrievalSampleRate(22050);
	QCOMPARE(s->isSeekable(), false);
	delete s;
    }

    void seekTrivial() {
	AudioReadStream *s = AudioReadStreamFactory::createReadStream(testsound());
	QVERIFY(s);
	QCOMPARE(s->seek(0), true);
	delete s;
    }

    void seekBeyondEnd() {
	AudioReadStream *s = AudioReadStreamFactory::createReadStream(testsound());
	QVERIFY(s);
	QCOMPARE(s->seek(100), false);
	delete s;
    }
    
    void readTwice() {
	AudioReadStream *s = AudioReadStreamFactory::createReadStream(testsound());
	QVERIFY(s);
	float firstPass[22], secondPass[22];
	size_t n = s->getInterleavedFrames(22, firstPass);
	QCOMPARE(n, size_t(20));
        QCOMPARE(s->seek(0), true);
	size_t m = s->getInterleavedFrames(22, secondPass);
	QCOMPARE(m, n);
        for (size_t i = 0; i < n; ++i) {
            QCOMPARE(firstPass[i], secondPass[i]);
        }
	delete s;
    }
    
    void unchangingSeeks() {
	AudioReadStream *s = AudioReadStreamFactory::createReadStream(testsound());
	QVERIFY(s);
	float frames[20];
	size_t n = s->getInterleavedFrames(20, frames);
	QCOMPARE(n, size_t(20));
        for (size_t i = 0; i < n; ++i) {
            QCOMPARE(s->seek(i), true);
            float f(-1.f);
            QCOMPARE(s->getInterleavedFrames(1, &f), size_t(1));
            QCOMPARE(f, frames[i]);
        }
	delete s;
    }
    
    void seekBeyondEndAndBack() {
	AudioReadStream *s = AudioReadStreamFactory::createReadStream(testsound());
	QVERIFY(s);
	QCOMPARE(s->seek(100), false);
	QCOMPARE(s->seek(0), true);
	float frames[4];
	size_t n = s->getInterleavedFrames(4, frames);
	QCOMPARE(n, size_t(4));
	QCOMPARE(frames[0], 32767.f/32768.f); // 16 bit file, so never quite 1
	QCOMPARE(frames[1], 0.f);
	QCOMPARE(frames[2], 0.f);
	QCOMPARE(frames[3], 0.f);
	delete s;
    }
    
    void seekToEnd() {
	AudioReadStream *s = AudioReadStreamFactory::createReadStream(testsound());
	QVERIFY(s);
        QCOMPARE(s->seek(16), true);
	float frames[20];
	size_t n = s->getInterleavedFrames(20, frames);
	QCOMPARE(n, size_t(4));
	QCOMPARE(frames[0], 0.f);
	QCOMPARE(frames[1], 0.f);
	QCOMPARE(frames[2], 0.f);
	QCOMPARE(frames[3], -1.f);
	delete s;
    }
};

}

#endif

	
