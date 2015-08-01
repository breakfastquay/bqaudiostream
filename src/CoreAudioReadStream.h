/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef BQ_CORE_AUDIO_READ_STREAM_H_
#define BQ_CORE_AUDIO_READ_STREAM_H_

#include "AudioReadStream.h"

#ifdef HAVE_COREAUDIO

namespace breakfastquay
{
    
class CoreAudioReadStream : public AudioReadStream
{
public:
    CoreAudioReadStream(std::string path);
    virtual ~CoreAudioReadStream();

    virtual std::string getError() const { return m_error; }

protected:
    virtual size_t getFrames(size_t count, float *frames);
    
    std::string m_path;
    std::string m_error;

    class D;
    D *m_d;
};

}

#endif

#endif
