/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    bqaudiostream

    A small library wrapping various audio file read/write
    implementations in C++.

    Copyright 2007-2022 Particular Programs Ltd.

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

#ifndef BQ_AUDIO_READ_STREAM_H
#define BQ_AUDIO_READ_STREAM_H

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
    virtual ~AudioReadStream();

    /**
     * Return any stream error message. Errors are reported by
     * throwing exceptions; after catching an exception, you may call
     * getError() to retrieve a more detailed, possibly human-readable
     * message.
     */
    virtual std::string getError() const { return ""; }

    /**
     * Return the number of channels in the stream.
     */
    size_t getChannelCount() const { return m_channelCount; }

    /**
     * Return the native sample rate of the stream.
     */
    size_t getSampleRate() const { return m_sampleRate; }

    /**
     * Return true if the audio stream is seekable to a specific frame
     * position within the source.
     *
     * This depends on the format and also on the retrieval sample
     * rate (see setRetrievalSampleRate() and
     * getRetrievalSampleRate()), as resampling streams are not
     * seekable.
     */
    bool isSeekable() const;

    /**
     * Return an estimate of the number of frames in the stream (at
     * its native sample rate) or zero if the stream can't provide
     * that information.
     *
     * For seekable streams (see isSeekable()) this is guaranteed to
     * return a true frame count. For other streams it may be
     * approximate, hence the name.
     */
    size_t getEstimatedFrameCount() const;

    /**
     * Set a sample rate at which audio data should be read from the
     * stream. The stream will resample if this differs from the
     * native rate of the stream (reported by getSampleRate()). The
     * default is to use the native rate of the stream.
     */
    void setRetrievalSampleRate(size_t);

    /**
     * Return the sample rate at which audio data will be read from
     * the stream. The stream will resample if this differs from the
     * native rate of the stream (reported by getSampleRate()).
     */
    size_t getRetrievalSampleRate() const;
    
    /**
     * Retrieve \count frames of audio data (that is, \count *
     * getChannelCount() samples) from the source and store in
     * \frames.  Return the number of frames actually retrieved; this
     * will differ from \count only when the end of stream is reached.
     * The region pointed to by \frames must contain enough space for
     * \count * getChannelCount() values.
     *
     * If a retrieval sample rate has been set, the audio will be
     * resampled to that rate (and \count refers to the number of
     * frames at the retrieval rate rather than the file's original
     * rate).
     *
     * May throw InvalidFileFormat if decoding fails.
     */
    size_t getInterleavedFrames(size_t count, float *frames);

    /**
     * Re-seek the stream to the requested audio frame position.
     * Return true on success, or false if the stream is not seekable
     * or the requested frame is out of range (beyond the end of the
     * stream).
     *
     * The stream is left in a valid state regardless of whether
     * seeking succeeds, but its position following a call that
     * returns false is not defined. If the stream is seekable, it
     * will be possible to seek it back into range and continue
     * reading even after a failed seek.
     */
    bool seek(size_t frame);

    /**
     * Return the track name or title, if any was found in the stream,
     * or an empty string otherwise.
     */
    virtual std::string getTrackName() const = 0;

    /**
     * Return the artist name, if any was found in the stream, or an
     * empty string otherwise.
     */
    virtual std::string getArtistName() const = 0;
    
protected:
    AudioReadStream();
    virtual size_t getFrames(size_t count, float *frames) = 0;
    virtual bool performSeek(size_t) { return false; }
    size_t m_channelCount;
    size_t m_sampleRate;
    size_t m_estimatedFrameCount;
    bool m_seekable;

private:
    int getResampledChunk(int count, float *frames);
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
//        std::cerr << "Registering stream builder: " << uri << std::endl;
    }
};

}

#endif
