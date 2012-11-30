/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef USE_ISO_MP3

#include "BasicMp3ReadStream.h"

#include "base/RingBuffer.h"

#include "iso-mp3/wrap/MP3Decoder.h"

#ifndef __GNUC__
#include <alloca.h>
#endif

//#define DEBUG_BASIC_MP3_READ_STREAM 1

namespace Turbot
{

static
AudioReadStreamBuilder<BasicMP3ReadStream>
oggbuilder(
    QString("http://breakfastquay.com/rdf/turbot/fileio/BasicMP3ReadStream"),
    QStringList() << "mp3"
    );

class BasicMP3ReadStream::D : public MP3DecoderCallback
{
public:
    D(BasicMP3ReadStream *rs) :
        m_rs(rs),
        m_decoder(0),
        m_buffer(0),
        m_finished(false) { }
    virtual ~D() {
	delete m_decoder;
        delete m_buffer;
    }

    BasicMP3ReadStream *m_rs;
    MP3Decoder *m_decoder;
    RingBuffer<float> *m_buffer;
    bool m_finished;

    // Callback methods

    void changeState(State s) {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        std::cerr << "changeState: " << s << std::endl;
#endif
        if (s == Finished || s == Failed) {
            m_finished = true;
        }
    }

    void setChannelCount(int c) {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        std::cerr << "setChannelCount: " << c << std::endl;
#endif
        m_rs->setChannelCount(c);
    }

    void setSampleRate(int r) {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        std::cerr << "setSampleRate: " << r << std::endl;
#endif
        m_rs->setSampleRate(r);
    }

    int acceptFrames(const short *interleaved, int total) {

        int channels = m_rs->getChannelCount();
        int n = total / channels;

#ifdef DEBUG_BASIC_MP3_READ_STREAM
        std::cerr << "acceptFrames: " << n << std::endl;
#endif

        sizeBuffer(getAvailableFrameCount() + n);
#ifdef __GNUC__
        float fi[n * channels];
#else
        float *fi = (float *)alloca(n * channels * sizeof(float));
#endif

        for (int i = 0; i < n * channels; ++i) {
            short s = interleaved[i];
	    fi[i] = float(s) / 32768.f;
        }

        m_buffer->write(fi, n * channels);
        return 0;
    }

    bool isFinished() const {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        std::cerr << "isFinished?: " << m_finished << std::endl;
#endif
        return m_finished;
    }

    int getAvailableFrameCount() const {
        if (!m_buffer) {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
            std::cerr << "getAvailableFrameCount: no buffer" << std::endl;
#endif
            return 0;
        }
        int n = m_buffer->getReadSpace() / m_rs->getChannelCount();
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        std::cerr << "getAvailableFrameCount: " << n << std::endl;
#endif
        return n;
    }

    void process() {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        std::cerr << "process: m_finished = " << m_finished << std::endl;
#endif
        if (m_finished) return;
        m_decoder->processBlock();
    }

    void sizeBuffer(int minFrames) {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        std::cerr << "sizeBuffer: " << minFrames << std::endl;
#endif
        int samples = minFrames * m_rs->getChannelCount();
        if (!m_buffer) {
            m_buffer = new RingBuffer<float>(samples);
        } else if (m_buffer->getSize() < samples) {
            m_buffer = m_buffer->resized(samples);
        }
    }
};

BasicMP3ReadStream::BasicMP3ReadStream(QString path) :
    m_path(path),
    m_d(new D(this))
{
    m_channelCount = 0;
    m_sampleRate = 0;

    if (!QFile(m_path).exists()) throw FileNotFound(m_path);

    m_d->m_decoder = new MP3Decoder(path.toLocal8Bit().data(), m_d);

    while ((m_channelCount == 0 || m_sampleRate == 0) && !m_d->m_finished) {
        m_d->process();
    }
}

BasicMP3ReadStream::~BasicMP3ReadStream()
{
    delete m_d;
}

void
BasicMP3ReadStream::setChannelCount(int c)
{
    m_channelCount = c;
}

void
BasicMP3ReadStream::setSampleRate(int r)
{
    m_sampleRate = r;
}

size_t
BasicMP3ReadStream::getFrames(size_t count, float *frames)
{
    if (!m_channelCount) return 0;
    if (count == 0) return 0;

    while (m_d->getAvailableFrameCount() < count) {
        if (m_d->isFinished()) break;
        m_d->process();
    }

    int avail = m_d->m_buffer->getReadSpace();
    int favail = avail / m_channelCount;
    int toRead = count;
    if (toRead > favail) toRead = favail;

    int got = m_d->m_buffer->read(frames, toRead * m_channelCount);
    int fgot = got / m_channelCount;

#ifdef DEBUG_BASIC_MP3_READ_STREAM
    std::cerr << "getFrames: " << fgot << std::endl;
#endif

    return fgot;
}

}

#endif
