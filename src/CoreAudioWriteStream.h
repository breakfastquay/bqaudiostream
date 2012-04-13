/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_CORE_AUDIO_WRITE_STREAM_H_
#define _TURBOT_CORE_AUDIO_WRITE_STREAM_H_

#include "AudioWriteStream.h"

#ifdef HAVE_COREAUDIO

namespace Turbot
{
    
class CoreAudioWriteStream : public AudioWriteStream
{
public:
    CoreAudioWriteStream(Target target);
    virtual ~CoreAudioWriteStream();

    virtual QString getError() const { return m_error; }

    virtual bool putInterleavedFrames(size_t count, float *frames);
    
protected:
    class D;
    D *m_d;
};

}

#endif

#endif
