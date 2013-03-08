/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef TEST_AUDIOSTREAM_READ_H
#define TEST_AUDIOSTREAM_READ_H

#include "audiostream/AudioReadStreamFactory.h"
#include "audiostream/AudioReadStream.h"

#include "base/Exceptions.h"

#include "AudioStreamTestData.h"

#include <cmath>

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

namespace Turbot {

static QString audioDir = "../../../resources/testfiles/audiostream";

class TestAudioStreamRead : public QObject
{
    Q_OBJECT

    const char *strOf(QString s) {
        return strdup(s.toLocal8Bit().data());
    }

private slots:
    void init()
    {
        if (!QDir(audioDir).exists()) {
            std::cerr << "ERROR: Audio test file directory \"" << audioDir << "\" does not exist" << std::endl;
            QVERIFY2(QDir(audioDir).exists(), "Audio test file directory not found");
        }
    }

    void read_data()
    {
        QTest::addColumn<QString>("audiofile");
        QStringList files = QDir(audioDir).entryList(QDir::Files);
        foreach (QString filename, files) {
            QTest::newRow(strOf(filename)) << filename;
        }
    }

    void read()
    {
        QFETCH(QString, audiofile);
        try {

            AudioReadStream *stream =
                AudioReadStreamFactory::createReadStream
                (audioDir + "/" + audiofile);

            stream->setRetrievalSampleRate(48000);
            int channels = stream->getChannelCount();
            AudioStreamTestData tdata(48000, channels);

            float *reference = tdata.getInterleavedData();
            int refsize = tdata.getFrameCount() * channels;

            float *test = new float[refsize];

            // The stream should give us exactly the expected number
            // of frames -- so we ask for one more, just to check we
            // don't get it!
            int read = stream->getInterleavedFrames
                (tdata.getFrameCount() + 1, test);
            
            QCOMPARE(read, tdata.getFrameCount());
            
            for (int c = 0; c < channels; ++c) {
                float maxdiff = 0.f;
                float totdiff = 0.f;
                for (int i = 0; i < read; ++i) {
                    float diff = fabsf(test[i * channels + c] -
                                       reference[i * channels + c]);
                    totdiff += diff;
                    if (diff > maxdiff) maxdiff = diff;
                }
                float meandiff = totdiff / read;
                std::cerr << "meandiff on channel " << c << ": " << meandiff << std::endl;
                std::cerr << "maxdiff on channel " << c << ": " << maxdiff << std::endl;
            }

        } catch (UnknownFileType &t) {
            QSKIP(strOf(QString("File format for \"%1\" not supported, skipping").arg(audiofile)), SkipSingle);
        }
    }
};

}

#endif

            
