/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef TEST_WAV_READ_WHILE_WRITING_H
#define TEST_WAV_READ_WHILE_WRITING_H

#include <QObject>
#include <QtTest>

#include "bqaudiostream/../src/SimpleWavFileReadStream.h"
#include "bqaudiostream/../src/SimpleWavFileWriteStream.h"

namespace breakfastquay {

class TestWavReadWhileWriting : public QObject
{
    Q_OBJECT

    void initBuf(std::vector<float> &buffer, int start, int n) {
        for (int i = 0; i < n; ++i) {
            float value = float(start + i) / 32767.f;
            buffer[i] = value;
        }
    }

    void zeroBuf(std::vector<float> &buffer) {
        for (int i = 0; i < int(buffer.size()); ++i) {
            buffer[i] = 0.f;
        }
    }
    
    bool checkBuf(const std::vector<float> &buffer, int start, int n) {
        for (int i = 0; i < n; ++i) {
            float value = float(start + i) / 32767.f;
            float diff = fabsf(buffer[i] - value);
            float threshold = 1.f / 65534.f;
            if (diff > threshold) {
                std::cerr << "checkBuf: at index " << i << " (start = "
                          << start << ", n = " << n << "), expected "
                          << value << ", found " << buffer[i]
                          << " (diff = " << diff << ", above threshold "
                          << threshold << ")" << std::endl;
                return false;
            }
        }
        return true;
    }
    
private slots:
    void readWhileWriting() {

        int bs = 1024;
        int channels = 2;
        std::vector<float> buffer(bs * channels, 0.f);
        
        AudioWriteStream::Target target("test-audiostream-readwhilewriting.wav",
                                        channels, 44100);

        SimpleWavFileWriteStream *ws = nullptr;
        SimpleWavFileReadStream *rs = nullptr;

        try {
            ws = new SimpleWavFileWriteStream(target);
        } catch (const std::exception &e) {
            std::cerr << "readWhileWriting: exception caught when creating write stream: " << e.what() << std::endl;
            throw;
        }
        QVERIFY(ws->getError() == std::string());

        try {
            rs = new SimpleWavFileReadStream(target.getPath());
        } catch (const std::exception &e) {
            std::cerr << "readWhileWriting: exception caught when creating read stream: " << e.what() << std::endl;
            delete ws;
            throw;
        }
        QVERIFY(rs->getError() == std::string());

        QCOMPARE(rs->getChannelCount(), size_t(channels));
        QCOMPARE(rs->getSampleRate(), size_t(44100));
        QVERIFY(rs->isSeekable());

        //!!! Contradicts the documentation, which says "For seekable
        //!!! streams this is guaranteed to return a true frame count"
        //!!! - probably this stream should not be seekable?
        QCOMPARE(rs->getEstimatedFrameCount(), size_t(0));

        QCOMPARE(rs->getInterleavedFrames(bs, buffer.data()), size_t(0));

        initBuf(buffer, 0, bs * channels);

        ws->putInterleavedFrames(bs, buffer.data());

        zeroBuf(buffer);

        QCOMPARE(rs->getInterleavedFrames(bs, buffer.data()), size_t(bs));

        checkBuf(buffer, 0, bs * channels);
        
        delete rs;
        delete ws;
    }

};


}

#endif
