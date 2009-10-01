/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_MP3_READ_STREAM_H_
#define _TURBOT_MP3_READ_STREAM_H_

#include "AudioReadStream.h"

#ifdef HAVE_MAD

namespace Turbot
{
    
class MP3ReadStream : public AudioReadStream
{
public:
    MP3ReadStream(QString path);
    virtual ~MP3ReadStream();

    virtual QString getError() const { return m_error; }

protected:
    virtual size_t getFrames(size_t count, float *frames);

    QString m_path;
    QString m_error;

    class D;
    D *m_d;
};

}

#endif

#endif
