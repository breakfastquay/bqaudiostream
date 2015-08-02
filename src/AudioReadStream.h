/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef BQ_AUDIO_READ_STREAM_H_
#define BQ_AUDIO_READ_STREAM_H_

#include <bqthingfactory/ThingFactory.h>
#include <bqvec/RingBuffer.h>

#include <string>
#include <vector>

namespace breakfastquay {

class Resampler;

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

    virtual ~AudioReadStream();

    virtual std::string getError() const { return ""; }

    size_t getChannelCount() const { return m_channelCount; }
    size_t getSampleRate() const { return m_sampleRate; } // source stream rate
    
    void setRetrievalSampleRate(size_t);
    size_t getRetrievalSampleRate() const;

    virtual std::string getTrackName() const { return ""; }
    virtual std::string getArtistName() const { return ""; }
    
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
    public ConcreteThingBuilder<T, AudioReadStream, std::string>
{
public:
    AudioReadStreamBuilder(std::string uri, std::vector<std::string> extensions) :
        ConcreteThingBuilder<T, AudioReadStream, std::string>(uri, extensions) {
        std::cerr << "Registering stream builder: " << uri << std::endl;
    }
};

}

#endif
