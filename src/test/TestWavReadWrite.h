/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef TEST_WAV_READ_WRITE_H
#define TEST_WAV_READ_WRITE_H

#include <QObject>
#include <QtTest>

#include "AudioReadStreamFactory.h"
#include "AudioReadStream.h"
#include "AudioWriteStreamFactory.h"
#include "AudioWriteStream.h"

#include "bqvec/Allocators.h"
#include "base/AudioLevel.h"

using namespace breakfastquay;

namespace Turbot {

class TestWavReadWrite : public QObject
{
    Q_OBJECT

    // This is a 44.1KHz, 32-bit float WAV, about 13 seconds of music
    static const char *testfile() { 
	static const char *f = "../../../resources/testfiles/clip.wav";
	return f;
    }
    static const char *outfile() { 
	static const char *f = "test-audiostream-out.wav";
	return f;
    }
    static const char *outfile_origrate() { 
	static const char *f = "test-audiostream-out-origrate.wav";
	return f;
    }

private slots:
    void readWriteResample() {
	
	// First read file into memory at normal sample rate

	AudioReadStream *rs = AudioReadStreamFactory::createReadStream(testfile());
	QVERIFY(rs);

	int cc = rs->getChannelCount();
	QCOMPARE(cc, 2);

	int rate = rs->getSampleRate();
	QCOMPARE(rate, 44100);

	int bs = 2048;
	int count = 0;
	int bufsiz = bs;
	float *buffer = allocate<float>(bs);

	while (1) {
	    if (count + bs > bufsiz) {
		buffer = reallocate<float>(buffer, bufsiz, bufsiz * 2);
		bufsiz *= 2;
	    }
	    int got = cc * rs->getInterleavedFrames(bs / cc, buffer + count);
	    count += got;
	    if (got < bs) break;
	}
	
	delete rs;
    
	// Re-open with resampling

	rs = AudioReadStreamFactory::createReadStream(testfile());

	QVERIFY(rs);

	rs->setRetrievalSampleRate(rate * 2);
    
	// Write resampled test file
    
	AudioWriteStream *ws = AudioWriteStreamFactory::createWriteStream
	    (outfile(), cc, rate * 2);

	QVERIFY(ws);
	
	float *block = allocate<float>(bs);

	while (1) {
	    int got = rs->getInterleavedFrames(bs / cc, block);
	    ws->putInterleavedFrames(got, block);
	    if (got < bs / cc) break;
	}

	delete ws;
	delete rs;
	ws = 0;

	// Read back resampled file at original rate and compare
    
        rs = AudioReadStreamFactory::createReadStream(outfile());

        QVERIFY(rs);
        QCOMPARE(rs->getSampleRate(), size_t(rate * 2));

        rs->setRetrievalSampleRate(rate);

        ws = AudioWriteStreamFactory::createWriteStream
            (outfile_origrate(), cc, rate);

        QVERIFY(ws);
    
        float error = AudioLevel::dB_to_ratio(-10);
        float warning = AudioLevel::dB_to_ratio(-25);
        float maxdiff = 0.f;
        float mda = 0.f, mdb = 0.f;
        int maxdiffindex = -1;

        count = 0;

        bool bad = false;

        while (1) {
            int got = rs->getInterleavedFrames(bs / cc, block);
            for (int i = 0; i < got * cc; ++i) {
                float a = block[i];
                float b = buffer[count + i];
                float diff = fabsf(a - b);
                if (diff > maxdiff &&
                    (count + i) > 10) { // first few samples are generally shaky
                    maxdiff = diff;
                    maxdiffindex = count + i;
                    mda = a;
                    mdb = b;
                }
            }
            count += got * cc;
            ws->putInterleavedFrames(got, block);
            if (got < bs / cc) break;
        }
        
        delete ws;
        delete rs;
        deallocate(block);
        deallocate(buffer);

        QString message = QString("Max diff is %1 (%2 dB) at index %3 (a = %4, b = %5) [error threshold %6 (%7 dB), warning threshold %8 (%9 dB)]")
            .arg(maxdiff)
            .arg(AudioLevel::ratio_to_dB(maxdiff))
            .arg(maxdiffindex)
            .arg(mda)
            .arg(mdb)
            .arg(error)
            .arg(AudioLevel::ratio_to_dB(error))
            .arg(warning)
            .arg(AudioLevel::ratio_to_dB(warning));

        QVERIFY2(maxdiff < error, message.toLocal8Bit().data());

        if (maxdiff > warning) {
            QWARN(message.toLocal8Bit().data());
        }	
    }
};

}

#endif
