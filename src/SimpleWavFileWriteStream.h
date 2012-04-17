/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_SIMPLE_WAV_FILE_WRITE_STREAM_H_
#define _TURBOT_SIMPLE_WAV_FILE_WRITE_STREAM_H_

#include "AudioWriteStream.h"

// If we have libsndfile, we shouldn't be using this class
#ifndef HAVE_LIBSNDFILE

#include <fstream>
#include <string>

namespace Turbot
{
    
class SimpleWavFileWriteStream : public AudioWriteStream
{
public:
    SimpleWavFileWriteStream(Target target);
    virtual ~SimpleWavFileWriteStream();

    virtual QString getError() const { return m_error; }

    virtual void putInterleavedFrames(size_t count, float *frames);
    
protected:
    int m_bitDepth;
    QString m_error;
    std::ofstream *m_file;

    void writeFormatChunk();
    std::string int2le(unsigned int value, unsigned int length);
    void putBytes(std::string);
    void putBytes(const unsigned char *, size_t);
};

}

#endif

#endif
