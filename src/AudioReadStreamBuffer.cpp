/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioReadStreamBuffer.h"

#include "AudioReadStreamFactory.h"
#include "AudioReadStream.h"

#include "base/RingBuffer.h"

#include "system/VectorOps.h"

namespace Turbot {

class AudioReadStreamBuffer::D
{
public:
    D(QString fileName, double bufferLengthSecs, int retrieveAtRate) :
	m_stream(0),
	m_buffer(0),
	m_interleaved(0)
    {
	m_stream = AudioReadStreamFactory::createReadStream(fileName);

	int rate = m_stream->getSampleRate();
	int channels = m_stream->getChannelCount();

	if (retrieveAtRate != 0 && retrieveAtRate != rate) {
	    rate = retrieveAtRate;
	    m_stream->setRetrievalSampleRate(rate);
	}

	int buflen = int(rate * bufferLengthSecs + 0.5);
	m_buffer = new RingBuffer<float>(buflen * channels);
	m_interleaved = new float[buflen * channels];

	rewind();
    }

    ~D() {
	delete m_stream;
	delete m_buffer;
	delete[] m_interleaved;
    }

    int getChannelCount() const {
	return m_stream->getChannelCount();
    }

    int getSampleRate() const {
	return m_stream->getSampleRate();
    }

    int getAvailableFrameCount() const {
	return m_buffer->getReadSpace() / getChannelCount();
    }

    int getFrames(int count, float **frames) const {
	int channels = getChannelCount();
	int available = getAvailableFrameCount();
	if (count > available) count = available;
	m_buffer->read(m_interleaved, count * channels);
	v_deinterleave(frames, m_interleaved, channels, count);

	//!!! signal to read thread

	return count;
    }

    //!!! todo: insert read thread here!

    void rewind() {
	//!!! sync!
    }

private:
    AudioReadStream *m_stream;
    RingBuffer<float> *m_buffer;
    float *m_interleaved;
};

AudioReadStreamBuffer::AudioReadStreamBuffer(QString fileName,
					     double bufferLengthSecs,
					     int retrieveAtRate) :
    m_d(new D(fileName, bufferLengthSecs, retrieveAtRate))
{
}

AudioReadStreamBuffer::~AudioReadStreamBuffer()
{
    delete m_d;
}

int
AudioReadStreamBuffer::getChannelCount() const
{
    return m_d->getChannelCount();
}

int
AudioReadStreamBuffer::getSampleRate() const
{
    return m_d->getSampleRate();
}

void
AudioReadStreamBuffer::rewind()
{
    m_d->rewind();
}

int
AudioReadStreamBuffer::getAvailableFrameCount() const
{
    return m_d->getAvailableFrameCount();
}

int
AudioReadStreamBuffer::getFrames(int count, float **frames)
{
    return m_d->getFrames(count, frames);
}

}



