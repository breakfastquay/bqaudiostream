/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_READ_STREAM_H_
#define _TURBOT_AUDIO_READ_STREAM_H_

#include "base/TurbotTypes.h"
#include "base/ThingFactory.h"

#include <string>
#include <vector>
#include <QUrl>

namespace Turbot {

/* Not thread-safe -- one per thread please. */

class AudioReadStream
{
public:
    class FileDRMProtected : virtual public std::exception
    {
    public:
        FileDRMProtected(std::string file) throw();
        virtual ~FileDRMProtected() throw() { }
        virtual const char *what() const throw();

    protected:
        std::string m_file;
    };

    virtual ~AudioReadStream() { }

    bool isOK() const { return (m_channelCount > 0); }

    virtual std::string getError() const { return ""; }

    size_t getChannelCount() const { return m_channelCount; }
    size_t getSampleRate() const { return m_sampleRate; }
    
    virtual size_t getInterleavedFrames(size_t count, float *frames) = 0;
    
protected:
    size_t m_channelCount;
    size_t m_sampleRate;
};

template <typename T>
class AudioReadStreamBuilder :
    public ConcreteThingBuilder<T, AudioReadStream, std::string>
{
public:
    AudioReadStreamBuilder(QUrl uri) :
        ConcreteThingBuilder<T, AudioReadStream, std::string>(uri) { }
};

}

#endif
