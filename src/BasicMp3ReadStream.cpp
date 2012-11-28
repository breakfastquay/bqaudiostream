/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef USE_ISO_MP3

#include "BasicMp3ReadStream.h"

#include "base/RingBuffer.h"

#include "iso-mp3/wrap/MP3Decoder.h"

#ifndef __GNUC__
#include <alloca.h>
#endif

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
    ~D() {
	delete m_decoder;
        delete m_buffer;
    }

    BasicMP3ReadStream *m_rs;
    MP3Decoder *m_decoder;
    RingBuffer<float> *m_buffer;
    bool m_finished;

    // Callback methods

    void changeState(State s) {
        std::cerr << "changeState: " << s << std::endl;
        if (s == Finished || s == Failed) {
            m_finished = true;
        }
    }

    void setChannelCount(int c) {
        std::cerr << "setChannelCount: " << c << std::endl;
        m_rs->setChannelCount(c);
    }

    void setSampleRate(int r) {
        std::cerr << "setSampleRate: " << r << std::endl;
        m_rs->setSampleRate(r);
    }

    int acceptFrames(const short *interleaved, int n) {

        std::cerr << "acceptFrames: " << n << std::endl;

        sizeBuffer(getAvailableFrameCount() + n);
        int channels = m_rs->getChannelCount();
#ifdef __GNUC__
        float fi[n * channels];
#else
        float *fi = (float *)alloca(n * channels * sizeof(float));
#endif
	for (long i = 0; i < n * channels; ++i) {
	    fi[i] = float(interleaved[i]) / 31768.f;
        }
        m_buffer->write(fi, n * channels);
        return 0;
    }

    bool isFinished() const {
        std::cerr << "isFinished?: " << m_finished << std::endl;
        return m_finished;
    }

    int getAvailableFrameCount() const {
        if (!m_buffer) {
            std::cerr << "getAvailableFrameCount: no buffer" << std::endl;
            return 0;
        }
        int n = m_buffer->getReadSpace() / m_rs->getChannelCount();
        std::cerr << "getAvailableFrameCount: " << n << std::endl;
        return n;
    }

    void process() {
        std::cerr << "process: m_finished = " << m_finished << std::endl;
        if (m_finished) return;
        m_decoder->processBlock();
    }

    void sizeBuffer(int minFrames) {
        std::cerr << "sizeBuffer: " << minFrames << std::endl;
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
    std::cerr << "setChannelCount: " << c << std::endl;
}

void
BasicMP3ReadStream::setSampleRate(int r)
{
    m_sampleRate = r;
    std::cerr << "setSampleRate: " << r << std::endl;
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

    int n = m_d->m_buffer->read(frames, count * m_channelCount);
    std::cerr << "getFrames: " << n << std::endl;
    return n;
}

}

#endif
