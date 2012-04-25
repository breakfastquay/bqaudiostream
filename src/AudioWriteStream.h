/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_FILE_WRITE_STREAM_H_
#define _TURBOT_AUDIO_FILE_WRITE_STREAM_H_

#include "base/TurbotTypes.h"
#include "base/ThingFactory.h"
#include "system/Debug.h"

#include <QString>

namespace Turbot {

/* Not thread-safe -- one per thread please. */

class AudioWriteStream
{
public:
    class Target {
    public:
        Target(QString path, size_t channelCount, size_t sampleRate) :
            m_path(path), m_channelCount(channelCount), m_sampleRate(sampleRate)
        { }

        QString getPath() const { return m_path; }
        size_t getChannelCount() const { return m_channelCount; }
        size_t getSampleRate() const { return m_sampleRate; }

    private:
        QString m_path;
        size_t m_channelCount;
        size_t m_sampleRate;
    };

    virtual ~AudioWriteStream() { }

    virtual QString getError() const { return ""; }

    QString getPath() const { return m_target.getPath(); }
    size_t getChannelCount() const { return m_target.getChannelCount(); }
    size_t getSampleRate() const { return m_target.getSampleRate(); }
    
    /**
     * May throw FileOperationFailed if encoding fails.
     */
    virtual void putInterleavedFrames(size_t count, float *frames) = 0;
    
protected:
    AudioWriteStream(Target t) : m_target(t) { }
    Target m_target;
};

template <typename T>
class AudioWriteStreamBuilder :
public ConcreteThingBuilder<T, AudioWriteStream, AudioWriteStream::Target>
{
public:
    AudioWriteStreamBuilder(QString uri, QStringList extensions) :
        ConcreteThingBuilder<T, AudioWriteStream, AudioWriteStream::Target>
        (uri, extensions) {
        std::cerr << "Registering stream builder: " << uri << std::endl;
    }
};

}

#endif
