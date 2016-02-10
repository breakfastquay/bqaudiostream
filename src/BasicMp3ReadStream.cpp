/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/*
    bqaudiostream

    A small library wrapping various audio file read/write
    implementations in C++.

    Copyright 2007-2015 Particular Programs Ltd.

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the names of Chris Cannam and
    Particular Programs Ltd shall not be used in advertising or
    otherwise to promote the sale, use or other dealings in this
    Software without prior written authorization.
*/

#ifdef USE_ISO_MP3

// WARNING This file is a test implementation only, not a useful decoder

#include "BasicMp3ReadStream.h"

#include "base/RingBuffer.h"

#include "iso-mp3/wrap/MP3Decoder.h"

#ifndef __GNUC__
#include <alloca.h>
#endif

//#define DEBUG_BASIC_MP3_READ_STREAM 1

namespace breakfastquay
{

static
AudioReadStreamBuilder<BasicMP3ReadStream>
oggbuilder(
    string("http://breakfastquay.com/rdf/turbot/audiostream/BasicMP3ReadStream"),
    vector<string>() << "mp3"
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
        cerr << "changeState: " << s << std::endl;
#endif
        if (s == Finished || s == Failed) {
            m_finished = true;
        }
    }

    void setChannelCount(int c) {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        cerr << "setChannelCount: " << c << std::endl;
#endif
        m_rs->setChannelCount(c);
    }

    void setSampleRate(int r) {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        cerr << "setSampleRate: " << r << std::endl;
#endif
        m_rs->setSampleRate(r);
    }

    int acceptFrames(const short *interleaved, int total) {

        int channels = m_rs->getChannelCount();
        int n = total / channels;

#ifdef DEBUG_BASIC_MP3_READ_STREAM
        cerr << "acceptFrames: " << n << std::endl;
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
        cerr << "isFinished?: " << m_finished << std::endl;
#endif
        return m_finished;
    }

    int getAvailableFrameCount() const {
        if (!m_buffer) {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
            cerr << "getAvailableFrameCount: no buffer" << std::endl;
#endif
            return 0;
        }
        int n = m_buffer->getReadSpace() / m_rs->getChannelCount();
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        cerr << "getAvailableFrameCount: " << n << std::endl;
#endif
        return n;
    }

    void process() {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        cerr << "process: m_finished = " << m_finished << std::endl;
#endif
        if (m_finished) return;
        m_decoder->processBlock();
    }

    void sizeBuffer(int minFrames) {
#ifdef DEBUG_BASIC_MP3_READ_STREAM
        cerr << "sizeBuffer: " << minFrames << std::endl;
#endif
        int samples = minFrames * m_rs->getChannelCount();
        if (!m_buffer) {
            m_buffer = new RingBuffer<float>(samples);
        } else if (m_buffer->getSize() < samples) {
            m_buffer = m_buffer->resized(samples);
        }
    }
};

BasicMP3ReadStream::BasicMP3ReadStream(string path) :
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
    cerr << "getFrames: " << fgot << std::endl;
#endif

    return fgot;
}

}

#endif
