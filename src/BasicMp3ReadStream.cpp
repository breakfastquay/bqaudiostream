/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef USE_ISO_MP3

#include "BasicMP3ReadStream.h"

#include "base/RingBuffer.h"

#include "iso-mp3/wrap/mp3decode.h"

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

class BasicMP3ReadStream::D
{
public:
    D(BasicMP3ReadStream *rs) :
        m_rs(rs),
        m_mp3dec(0),
        m_fishSound(0),
        m_buffer(0),
        m_finished(false) { }
    ~D() {
	delete m_mp3dec;
        delete m_buffer;
    }

    BasicMP3ReadStream *m_rs;
    MP3Decoder *m_mp3dec;
    RingBuffer<float> *m_buffer;
    bool m_finished;

    bool isFinished() const {
        return m_finished;
    }

    int getAvailableFrameCount() const {
        if (!m_buffer) return 0;
        return m_buffer->getReadSpace() / m_rs->getChannelCount();
    }

    void process() {
        if (m_finished) return;
        if (m_mp3dec->process(1024) <= 0) {
            m_finished = true;
        }
    }

    void sizeBuffer(int minFrames) {
        int samples = minFrames * m_rs->getChannelCount();
        if (!m_buffer) {
            m_buffer = new RingBuffer<float>(samples);
        } else if (m_buffer->getSize() < samples) {
            m_buffer = m_buffer->resized(samples);
        }
    }

    int acceptFrames(short *interleaved, long n) {

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
};

BasicMP3ReadStream::BasicMP3ReadStream(QString path) :
    m_path(path),
    m_d(new D(this))
{
    m_channelCount = 0;
    m_sampleRate = 0;

    if (!QFile(m_path).exists()) throw FileNotFound(m_path);

    if (!m_d->m_mp3dec = new MP3Decoder(path.toLocal8Bit().data())) {
	m_error = QString("File \"%1\" is not a valid MP3 file.").arg(path);
	throw InvalidFileFormat(m_path);
    }

//    mp3dec_set_read_callback(m_d->m_mp3dec, -1, D::acceptPacketStatic, m_d);

    // initialise m_channelCount
//!!!???
/*
    while (m_channelCount == 0 && !m_d->m_finished) {
        m_d->process();
    }
*/
}

BasicMP3ReadStream::~BasicMP3ReadStream()
{
    delete m_d;
}

size_t
BasicMP3ReadStream::getFrames(size_t count, float *frames)
{
    if (!m_channelCount) return 0;
    if (count == 0) return 0;

    while (m_d->getAvailableFrameCount() < count) {
        if (m_d->isFinished()) break;
        m_d->readNextBlock();
    }

    return m_d->m_buffer->read(frames, count * m_channelCount);
}

}

#endif
#endif
