/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_MAD

#include "MP3ReadStream.h"

#include "base/RingBuffer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mad.h>

namespace Turbot
{

static
AudioReadStreamBuilder<MP3ReadStream>
mp3builder(
    QString("http://breakfastquay.com/rdf/turbot/fileio/MP3ReadStream"),
    QStringList() << "mp3"
    );

class MP3ReadStream::D
{
public:
    D(MP3ReadStream *rs) :
        m_rs(rs),
        m_buffer(0),
        m_finished(false) { }
    ~D() {
        delete m_buffer;
    }

    MP3ReadStream *m_rs;
    RingBuffer<float> *m_buffer;
    bool m_finished;

    bool isFinished() const {
        return m_finished;
    }

    int getAvailableFrameCount() const {
        if (!m_buffer) return 0;
        return m_buffer->getReadSpace() / m_rs->getChannelCount();
    }

    void readNextBlock() {
        if (m_finished) return;
/*
        if (oggz_read(m_oggz, 1024) <= 0) {
            m_finished = true;
        }
*/
    }

    void sizeBuffer(int minFrames) {
        int samples = minFrames * m_rs->getChannelCount();
        if (!m_buffer) {
            m_buffer = new RingBuffer<float>(samples);
        } else if (m_buffer->getSize() < samples) {
            m_buffer = m_buffer->resized(samples);
        }
    }

};

MP3ReadStream::MP3ReadStream(QString path) :
    m_path(path),
    m_d(new D(this))
{
    m_channelCount = 0;
    m_sampleRate = 0;

    //!!!
}

MP3ReadStream::~MP3ReadStream()
{
    delete m_d;
}

size_t
MP3ReadStream::getFrames(size_t count, float *frames)
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
