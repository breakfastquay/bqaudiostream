/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_WAV_FILE_READ_STREAM_H_
#define _TURBOT_WAV_FILE_READ_STREAM_H_

#include "AudioReadStream.h"

#ifdef HAVE_LIBSNDFILE

#include <sndfile.h>

namespace Turbot
{
    
class WavFileReadStream : public AudioReadStream
{
public:
    WavFileReadStream(std::string path);
    virtual ~WavFileReadStream();

    virtual std::string getError() const { return m_error; }

    virtual size_t getInterleavedFrames(size_t count, float *frames);

    static std::vector<std::string> getSupportedFileExtensions();
    
    static QUrl getUri();
    
protected:
    SF_INFO m_fileInfo;
    SNDFILE *m_file;

    std::string m_path;
    std::string m_error;

    size_t m_offset;
};

}

#endif

#endif
