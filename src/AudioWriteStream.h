/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef BQ_AUDIO_FILE_WRITE_STREAM_H_
#define BQ_AUDIO_FILE_WRITE_STREAM_H_

#include "base/TurbotTypes.h"
#include "base/ThingFactory.h"
#include "system/Debug.h"

#include <string>

namespace breakfastquay {

/* Not thread-safe -- one per thread please. */

class AudioWriteStream
{
public:
    class Target {
    public:
        Target(std::string path, size_t channelCount, size_t sampleRate) :
            m_path(path), m_channelCount(channelCount), m_sampleRate(sampleRate)
        { }

        std::string getPath() const { return m_path; }
        size_t getChannelCount() const { return m_channelCount; }
        size_t getSampleRate() const { return m_sampleRate; }

    private:
        std::string m_path;
        size_t m_channelCount;
        size_t m_sampleRate;
    };

    virtual ~AudioWriteStream() { }

    virtual std::string getError() const { return ""; }

    std::string getPath() const { return m_target.getPath(); }
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
    AudioWriteStreamBuilder(std::string uri, std::vector<std::string> extensions) :
        ConcreteThingBuilder<T, AudioWriteStream, AudioWriteStream::Target>
        (uri, extensions) {
        std::cerr << "Registering stream builder: " << uri << std::endl;
    }
};

}

#endif
