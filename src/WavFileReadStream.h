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
    WavFileReadStream(QString path);
    virtual ~WavFileReadStream();

    virtual QString getError() const { return m_error; }

    virtual size_t getInterleavedFrames(size_t count, float *frames);
    
protected:
    SF_INFO m_fileInfo;
    SNDFILE *m_file;

    QString m_path;
    QString m_error;

    size_t m_offset;

    static AudioReadStreamBuilder<WavFileReadStream> m_builder;
};

}

#endif

#endif
