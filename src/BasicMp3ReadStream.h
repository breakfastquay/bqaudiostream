/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_BASIC_MP3_READ_STREAM_H_
#define _TURBOT_BASIC_MP3_READ_STREAM_H_

#include "AudioReadStream.h"

#ifdef USE_ISO_MP3

namespace Turbot
{
    
class BasicMP3ReadStream : public AudioReadStream
{
public:
    BasicMP3ReadStream(QString path);
    virtual ~BasicMP3ReadStream();

    virtual QString getError() const { return m_error; }

protected:
    virtual size_t getFrames(size_t count, float *frames);
    virtual void setChannelCount(int c);
    virtual void setSampleRate(int r);

    QString m_path;
    QString m_error;

    class D;
    D *m_d;
};

}

#endif

#endif
