/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef TEST_WAV_READ_WHILE_WRITING_H
#define TEST_WAV_READ_WHILE_WRITING_H

#include <QObject>
#include <QtTest>

#include "bqaudiostream/AudioReadStreamFactory.h"
#include "bqaudiostream/AudioReadStream.h"
#include "bqaudiostream/AudioWriteStreamFactory.h"
#include "bqaudiostream/AudioWriteStream.h"

#include <thread>
#include <chrono>

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
    void readWhileWritingNoWait() {

        int bs = 1024;
        int channels = 2;
        int rate = 44100;
        std::vector<float> buffer(bs * channels, 0.f);
        std::string file = "test-audiostream-readwhilewriting.wav";
        
        auto ws = AudioWriteStreamFactory::createWriteStream(file, channels, rate);
        QVERIFY(ws->getError() == std::string());

        auto rs = AudioReadStreamFactory::createReadStream(file);
        QVERIFY(rs->getError() == std::string());

        QCOMPARE(rs->getChannelCount(), size_t(channels));
        QCOMPARE(rs->getSampleRate(), size_t(44100));
        QVERIFY(rs->hasIncrementalSupport());
        QVERIFY(!rs->isSeekable());
        QCOMPARE(rs->getEstimatedFrameCount(), size_t(0));

        QCOMPARE(rs->getInterleavedFrames(bs, buffer.data()), size_t(0));

        initBuf(buffer, 0, bs * channels);

        ws->putInterleavedFrames(bs, buffer.data());

        zeroBuf(buffer);

        QCOMPARE(rs->getInterleavedFrames(bs, buffer.data()), size_t(bs));

        QVERIFY(checkBuf(buffer, 0, bs * channels));
        
        delete rs;
        delete ws;
    }

    void readWhileWritingWithWait() {

        int bs = 1024;
        int channels = 2;
        int rate = 44100;
        std::vector<float> readbuf(bs * channels, 0.f);
        std::vector<float> writebuf(bs * channels, 0.f);
        std::string file = "test-audiostream-readwhilewriting.wav";
        
        auto ws = AudioWriteStreamFactory::createWriteStream(file, channels, rate);
        QVERIFY(ws->getError() == std::string());

        auto rs = AudioReadStreamFactory::createReadStream(file);
        QVERIFY(rs->getError() == std::string());

        QCOMPARE(rs->getChannelCount(), size_t(channels));
        QCOMPARE(rs->getSampleRate(), size_t(44100));
        QVERIFY(rs->hasIncrementalSupport());

        rs->setIncrementalTimeouts(20, 200);

        initBuf(writebuf, 0, bs * channels);
        
        auto writer = [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            ws->putInterleavedFrames(bs/2, writebuf.data());
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            ws->putInterleavedFrames(bs/2, writebuf.data() + (bs/2) * channels);
        };
        
        QCOMPARE(rs->getInterleavedFrames(bs, readbuf.data()), size_t(0));

        rs->setIncrementalTimeouts(20, 1000);
        
        std::thread writeThread(writer);
        
        QCOMPARE(rs->getInterleavedFrames(bs, readbuf.data()), size_t(bs));

        QVERIFY(checkBuf(readbuf, 0, bs * channels));

        writeThread.join();
        
        delete rs;
        delete ws;
    }

};


}

#endif
