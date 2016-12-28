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

#include "AudioReadStream.h"

#include "bqresample/Resampler.h"

#include <cmath>

using namespace std;

namespace breakfastquay
{
	
AudioReadStream::FileDRMProtected::FileDRMProtected(string file) throw() :
    m_file(file)
{
    m_what = "File \"" + m_file + "\" is protected by DRM";
}

const char *
AudioReadStream::FileDRMProtected::what() const throw()
{
    return m_what.c_str();
}

AudioReadStream::AudioReadStream() :
    m_channelCount(0),
    m_sampleRate(0),
    m_retrievalRate(0),
    m_totalFileFrames(0),
    m_totalRetrievedFrames(0),
    m_resampler(0),
    m_resampleBuffer(0)
{
}

AudioReadStream::~AudioReadStream()
{
    delete m_resampler;
    delete m_resampleBuffer;
}

void
AudioReadStream::setRetrievalSampleRate(size_t rate)
{
    m_retrievalRate = rate;
}

size_t
AudioReadStream::getRetrievalSampleRate() const
{
    if (m_retrievalRate == 0) return m_sampleRate;
    else return m_retrievalRate;
}

size_t
AudioReadStream::getInterleavedFrames(size_t count, float *frames)
{
    if (m_retrievalRate == 0 ||
        m_retrievalRate == m_sampleRate ||
        m_channelCount == 0) {
        return getFrames(count, frames);
    }

    int samples = count * m_channelCount;

    if (!m_resampler) {
        Resampler::Parameters params;
        params.quality = Resampler::Best;
        params.initialSampleRate = m_sampleRate;
        m_resampler = new Resampler(params, m_channelCount);
        m_resampleBuffer = new RingBuffer<float>(samples * 2);
    }

    float ratio = float(m_retrievalRate) / float(m_sampleRate);
    int fileFrames = int(ceil(count / ratio));
    bool finished = false;

    float *in  = allocate<float>(fileFrames * m_channelCount);
    float *out = allocate<float>((count + 1) * m_channelCount);

    while (m_resampleBuffer->getReadSpace() < samples) {

        int fileFramesRemaining =
            int(ceil((samples - m_resampleBuffer->getReadSpace())
                     / (m_channelCount * ratio)));

        int got = 0;

        if (!finished) {
            got = getFrames(fileFramesRemaining, in);
            m_totalFileFrames += got;
            if (got < fileFramesRemaining) {
                finished = true;
            }
        } else {
            v_zero(in, fileFramesRemaining * m_channelCount);
            got = fileFramesRemaining;
        }

        if (got > 0) {
            int resampled = m_resampler->resampleInterleaved
                (out, count + 1, in, got, ratio, finished);

            if (m_resampleBuffer->getWriteSpace() < resampled * m_channelCount) {
                m_resampleBuffer = m_resampleBuffer->resized
                    (m_resampleBuffer->getReadSpace() + resampled * m_channelCount);
            }
        
            m_resampleBuffer->write(out, resampled * m_channelCount);
        }
    }

    deallocate(in);
    deallocate(out);

    int toReturn = samples;

    int available = (int(m_totalFileFrames * ratio) - m_totalRetrievedFrames) *
        m_channelCount;
    
    if (toReturn > available) toReturn = available;

    m_totalRetrievedFrames += toReturn;

    return m_resampleBuffer->read(frames, toReturn) / m_channelCount;
}

}

