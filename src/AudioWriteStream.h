/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_FILE_WRITE_STREAM_H_
#define _TURBOT_AUDIO_FILE_WRITE_STREAM_H_

#include "base/TurbotTypes.h"

#include <QString>

namespace Turbot {

/* Not thread-safe -- one per thread please. */

class AudioWriteStream
{
public:
    virtual ~AudioWriteStream() { }

    bool isOK() const { return (m_channelCount > 0); }

    virtual QString getError() const { return ""; }

    size_t getChannelCount() const { return m_channelCount; }
    size_t getSampleRate() const { return m_sampleRate; }
    
    virtual bool putInterleavedFrames(size_t count, float *frames) = 0;
    
protected:
    AudioWriteStream(size_t channelCount, size_t sampleRate) :
        m_channelCount(channelCount), m_sampleRate(sampleRate) { }
    size_t m_channelCount;
    size_t m_sampleRate;
};

}

#endif
