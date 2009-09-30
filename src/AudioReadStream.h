/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_READ_STREAM_H_
#define _TURBOT_AUDIO_READ_STREAM_H_

#include "base/TurbotTypes.h"
#include "base/ThingFactory.h"

#include "system/Debug.h"

namespace Turbot {

/* Not thread-safe -- one per thread please. */

class AudioReadStream
{
public:
    class FileDRMProtected : virtual public std::exception
    {
    public:
        FileDRMProtected(QString file) throw();
        virtual ~FileDRMProtected() throw() { }
        virtual const char *what() const throw();

    protected:
        QString m_file;
    };

    virtual ~AudioReadStream() { }

    bool isOK() const { return (m_channelCount > 0); }

    virtual QString getError() const { return ""; }

    size_t getChannelCount() const { return m_channelCount; }
    size_t getSampleRate() const { return m_sampleRate; }
    
    virtual size_t getInterleavedFrames(size_t count, float *frames) = 0;
    
protected:
    size_t m_channelCount;
    size_t m_sampleRate;
};

template <typename T>
class AudioReadStreamBuilder :
    public ConcreteThingBuilder<T, AudioReadStream, QString>
{
public:
    AudioReadStreamBuilder(QUrl uri, QStringList extensions) :
        ConcreteThingBuilder<T, AudioReadStream, QString>(uri, extensions) {
        std::cerr << "Registering stream builder: " << uri << std::endl;
    }
};

}

#endif
