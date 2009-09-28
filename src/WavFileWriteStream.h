/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_WAV_FILE_WRITE_STREAM_H_
#define _TURBOT_WAV_FILE_WRITE_STREAM_H_

#include "AudioFileWriteStream.h"

#ifdef HAVE_LIBSNDFILE

#include <sndfile.h>

namespace Turbot
{
    
class WavFileWriteStream : public AudioFileWriteStream
{
public:
    WavFileWriteStream(std::string path, size_t channelCount, size_t sampleRate);
    virtual ~WavFileWriteStream();

    virtual std::string getError() const { return m_error; }

    virtual bool putInterleavedFrames(size_t count, float *frames);
    
protected:
    SF_INFO m_fileInfo;
    SNDFILE *m_file;

    std::string m_path;
    std::string m_error;
};

}

#endif

#endif
