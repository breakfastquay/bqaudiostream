/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef BQ_CORE_AUDIO_WRITE_STREAM_H_
#define BQ_CORE_AUDIO_WRITE_STREAM_H_

#include "AudioWriteStream.h"

#ifdef HAVE_COREAUDIO

namespace breakfastquay
{
    
class CoreAudioWriteStream : public AudioWriteStream
{
public:
    CoreAudioWriteStream(Target target);
    virtual ~CoreAudioWriteStream();

    virtual std::string getError() const { return m_error; }

    virtual void putInterleavedFrames(size_t count, float *frames);
    
protected:
    std::string m_error;

    class D;
    D *m_d;
};

}

#endif

#endif
