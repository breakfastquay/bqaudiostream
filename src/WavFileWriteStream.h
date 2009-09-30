/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_WAV_FILE_WRITE_STREAM_H_
#define _TURBOT_WAV_FILE_WRITE_STREAM_H_

#include "AudioWriteStream.h"

#ifdef HAVE_LIBSNDFILE

#include <sndfile.h>

namespace Turbot
{
    
class WavFileWriteStream : public AudioWriteStream
{
public:
    WavFileWriteStream(QString path, size_t channelCount, size_t sampleRate);
    virtual ~WavFileWriteStream();

    virtual QString getError() const { return m_error; }

    virtual bool putInterleavedFrames(size_t count, float *frames);
    
protected:
    SF_INFO m_fileInfo;
    SNDFILE *m_file;

    QString m_path;
    QString m_error;
};

}

#endif

#endif
