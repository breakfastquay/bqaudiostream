/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_QUICK_TIME_READ_STREAM_H_
#define _TURBOT_QUICK_TIME_READ_STREAM_H_

#include "AudioReadStream.h"

#ifdef HAVE_QUICKTIME

namespace Turbot
{
    
class QuickTimeReadStream : public AudioReadStream
{
public:
    QuickTimeReadStream(std::string path);
    virtual ~QuickTimeReadStream();

    virtual std::string getError() const { return m_error; }

    virtual size_t getInterleavedFrames(size_t count, float *frames);

    static std::vector<std::string> getSupportedFileExtensions();
    
    static QUrl getUri();
    
protected:
    std::string m_path;
    std::string m_error;

    class D;
    D *m_d;

    size_t m_offset;
};

}

#endif

#endif
