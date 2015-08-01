/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioReadStream.h"
#include "system/Debug.h"
#include "dsp/Resampler.h"

using namespace std;

namespace breakfastquay
{
	
AudioReadStream::FileDRMProtected::FileDRMProtected(string file) throw() :
    m_file(file)
{
    cerr << "ERROR: File is DRM protected: " << file << std::endl;
}

const char *
AudioReadStream::FileDRMProtected::what() const throw()
{
    return string("File \"%1\" is protected by DRM")
        .arg(m_file).toLocal8Bit().data();
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
        m_resampler = new Resampler(Resampler::Best, m_channelCount);
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
                (in, out, got, ratio, finished);

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

