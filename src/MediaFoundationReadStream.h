/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef TURBOT_MEDIAFOUNDATION_READ_STREAM_H
#define TURBOT_MEDIAFOUNDATION_READ_STREAM_H

#include "AudioReadStream.h"

#ifdef HAVE_MEDIAFOUNDATION
 
namespace breakfastquay
{
    
class MediaFoundationReadStream : public AudioReadStream
{
public:
    MediaFoundationReadStream(std::string path);
    virtual ~MediaFoundationReadStream();

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
