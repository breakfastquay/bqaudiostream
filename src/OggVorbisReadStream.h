/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef BQ_OGG_VORBIS_READ_STREAM_H_
#define BQ_OGG_VORBIS_READ_STREAM_H_

#include "AudioReadStream.h"

#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND

namespace breakfastquay
{
    
class OggVorbisReadStream : public AudioReadStream
{
public:
    OggVorbisReadStream(std::string path);
    virtual ~OggVorbisReadStream();

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

#endif
