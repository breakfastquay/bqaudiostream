/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioReadStream.h"
#include "system/Debug.h"
#include "dsp/Resampler.h"

namespace Turbot
{
	
AudioReadStream::FileDRMProtected::FileDRMProtected(QString file) throw() :
    m_file(file)
{
    std::cerr << "ERROR: File is DRM protected: " << file << std::endl;
}

const char *
AudioReadStream::FileDRMProtected::what() const throw()
{
    return QString("File \"%1\" is protected by DRM")
        .arg(m_file).toLocal8Bit().data();
}

AudioReadStream::AudioReadStream() :
    m_channelCount(0),
    m_sampleRate(0),
    m_retrievalRate(0),
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

    size_t samples = count * m_channelCount;

    if (!m_resampler) {
        m_resampler = new Resampler(Resampler::Best, m_channelCount);
        m_resampleBuffer = new RingBuffer<float>(samples * 2);
    }

    bool finished = false;

    while (m_resampleBuffer->getReadSpace() < samples && !finished) {

        float ratio = float(m_retrievalRate) / float(m_sampleRate);
        size_t req = size_t(ceil(count / ratio));

        float *in  = allocate<float>(req * m_channelCount);
        float *out = allocate<float>((count + 1) * m_channelCount);

        size_t got = getFrames(req, in);
    
        if (got < req) {
            finished = true;
        }

        if (got > 0) {
            int resampled = m_resampler->resampleInterleaved
                (in, out, got, ratio, got < req);

            if (m_resampleBuffer->getWriteSpace() < resampled * m_channelCount) {
                m_resampleBuffer = m_resampleBuffer->resized
                    (m_resampleBuffer->getReadSpace() + resampled * m_channelCount);
            }
        
            m_resampleBuffer->write(out, resampled * m_channelCount);
        }

        deallocate(in);
        deallocate(out);
    }

    return m_resampleBuffer->read(frames, samples) / m_channelCount;
}

}

