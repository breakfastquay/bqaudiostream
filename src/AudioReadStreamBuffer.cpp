/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioReadStreamBuffer.h"

#include "AudioReadStreamFactory.h"
#include "AudioReadStream.h"

#include "base/RingBuffer.h"

#include "system/VectorOps.h"
#include "system/Thread.h"

namespace Turbot {

class AudioReadStreamBuffer::D
{
    class ReadThread : public Thread
    {
    public:
	ReadThread(D *d) : 
	    m_d(d),
	    m_condition("AudioReadStreamBuffer::ReadThread"),
	    m_abandoned(false) { }
	~ReadThread() { }

	void signal() {
	    m_condition.signal();
	}
	void signalLocked() {
	    m_condition.lock();
	    m_condition.signal();
	    m_condition.unlock();
	}
	void abandon() {
	    m_abandoned = true;
	}
	
	virtual void run() {
	    m_condition.lock();
	    while (true) {
		if (m_d->m_buffer->getWriteSpace() == 0) {
		    m_condition.wait(500000);
		}
		if (m_abandoned) break;
		int channels = m_d->m_stream->getChannelCount();
		int wanted = m_d->m_buffer->getWriteSpace() / channels;
		if (ws > 0) {
		    float *frames = allocate<float>(wanted * channels);
		    size_t got = m_d->m_stream->getInterleavedFrames
			(wanted, frames);
		    //... if got != wanted, end of stream reached
		    m_d->m_buffer->write(frames, got * channels);
		    deallocate(frames);
		}
	    }
	}

    private:
	D *m_d;
	Condition m_condition;
	bool m_abandoned;
    };

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

	m_thread = new ReadThread(this);
	m_thread->start();

	rewind();
    }

    ~D() {
	m_thread->abandon();
	m_thread->signalLocked();
	m_thread->wait();
	delete m_thread;

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
	m_thread->signal();
	return count;
    }

    void rewind() {
	//!!! sync!
    }

protected:
    AudioReadStream *m_stream;
    RingBuffer<float> *m_buffer;
    float *m_interleaved;
    ReadThread *m_thread;
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



