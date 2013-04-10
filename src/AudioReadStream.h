/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_READ_STREAM_H_
#define _TURBOT_AUDIO_READ_STREAM_H_

#include "base/TurbotTypes.h"
#include "base/ThingFactory.h"
#include "base/RingBuffer.h"

#include "system/Debug.h"

namespace Turbot {

class Resampler;

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

    virtual ~AudioReadStream();

    virtual QString getError() const { return ""; }

    size_t getChannelCount() const { return m_channelCount; }
    size_t getSampleRate() const { return m_sampleRate; } // source stream rate
    
    void setRetrievalSampleRate(size_t);
    size_t getRetrievalSampleRate() const;

    /**
     * Retrieve \count frames of audio data (that is, \count *
     * getChannelCount() samples) from the source and store in
     * \frames.  Return the number of frames actually retrieved; this
     * will differ from \count only when the end of stream is reached.
     *
     * If a retrieval sample rate has been set, the audio will be
     * resampled to that rate (and \count refers to the number of
     * frames at the retrieval rate rather than the file's original
     * rate).
     *
     * May throw InvalidFileFormat if decoding fails.
     */
    size_t getInterleavedFrames(size_t count, float *frames);
    
protected:
    AudioReadStream();
    virtual size_t getFrames(size_t count, float *frames) = 0;
    size_t m_channelCount;
    size_t m_sampleRate;
    size_t m_retrievalRate;
    size_t m_totalFileFrames;
    size_t m_totalRetrievedFrames;
    Resampler *m_resampler;
    RingBuffer<float> *m_resampleBuffer;
};

template <typename T>
class AudioReadStreamBuilder :
    public ConcreteThingBuilder<T, AudioReadStream, QString>
{
public:
    AudioReadStreamBuilder(QString uri, QStringList extensions) :
        ConcreteThingBuilder<T, AudioReadStream, QString>(uri, extensions) {
        std::cerr << "Registering stream builder: " << uri << std::endl;
    }
};

}

#endif
