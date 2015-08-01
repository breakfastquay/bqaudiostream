/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef BQ_WAV_FILE_WRITE_STREAM_H_
#define BQ_WAV_FILE_WRITE_STREAM_H_

#include "AudioWriteStream.h"

#ifdef HAVE_LIBSNDFILE

#include <sndfile.h>

namespace breakfastquay
{
    
class WavFileWriteStream : public AudioWriteStream
{
public:
    WavFileWriteStream(Target target);
    virtual ~WavFileWriteStream();

    virtual std::string getError() const { return m_error; }

    virtual void putInterleavedFrames(size_t count, float *frames);
    
protected:
    SF_INFO m_fileInfo;
    SNDFILE *m_file;

    std::string m_error;
};

}

#endif

#endif
