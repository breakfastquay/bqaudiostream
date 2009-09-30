/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_DIRECTSHOW_READ_STREAM_H_
#define _TURBOT_DIRECTSHOW_READ_STREAM_H_

#include "AudioReadStream.h"

#ifdef HAVE_DIRECTSHOW
 
namespace Turbot
{
    
class DirectShowReadStream : public AudioReadStream
{
public:
    DirectShowReadStream(QString path);
    virtual ~DirectShowReadStream();

    virtual QString getError() const { return m_error; }

    virtual size_t getInterleavedFrames(size_t count, float *frames);
    
protected:
    QString m_path;
    QString m_error;

    class D;
    D *m_d;

    static AudioReadStreamBuilder<DirectShowReadStream> m_builder;
};

}

#endif

#endif
