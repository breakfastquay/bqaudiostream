/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_OGGZ_AND_FISHSOUND

#include "OggVorbisReadStream.h"

#include <oggz/oggz.h>
#include <fishsound/fishsound.h>

#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cmath>

namespace Turbot
{

static AudioReadStreamBuilder<OggVorbisReadStream>
builder(OggVorbisReadStream::getUri());

QUrl
OggVorbisReadStream::getUri()
{
    return QUrl("http://breakfastquay.com/rdf/turbot/fileio/OggVorbisReadStream");
}

class OggVorbisReadStream::D
{
public:
    D(OggVorbisReadStream *rs) : m_rs(rs), m_oggz(0), m_fishSound(0) { }
    ~D() {
	if (m_fishSound) fish_sound_delete(m_fishSound);
	if (m_oggz) oggz_close(m_oggz);
    }

    OggVorbisReadStream *m_rs;
    FishSound *m_fishSound;
    OGGZ *m_oggz;
 
    static int readPacket(OGGZ *, ogg_packet *, long, void *);
    static int acceptFrames(FishSound *, float **, long, void *);
};

OggVorbisReadStream::OggVorbisReadStream(std::string path) :
    m_path(path),
    m_d(new D(this))
{
    m_channelCount = 0;
    m_sampleRate = 0;

    if (!(m_d->m_oggz = oggz_open(path.c_str(), OGGZ_READ))) {
	m_error = QString("File %1 is not an OGG file.").arg(path);
	return;
    }

    FishSoundInfo fsinfo;
    m_d->m_fishSound = fish_sound_new(FISH_SOUND_DECODE, &fsinfo);

    fish_sound_set_decoded_callback(m_d->m_fishSound, D::acceptFrames, m_d);
    oggz_set_read_callback(m_d->m_oggz, -1, D::readPacket, m_d);

/*!!!
    while (oggz_read(oggz, 1024) > 0);

    fish_sound_delete(m_fishSound);
    m_fishSound = 0;
    oggz_close(oggz);

    if (isDecodeCacheInitialised()) finishDecodeCache();

    if (showProgress) {
	delete m_progress;
	m_progress = 0;
    }
*/
}

OggVorbisReadStream::~OggVorbisReadStream()
{
    delete m_d;
}

size_t
OggVorbisReadStream::getInterleavedFrames(size_t count, float *frames)
{
    if (!m_channelCount) return 0;
    if (count == 0) return 0;

/*!!!
    m_d->buffer.mBuffers[0].mDataByteSize =
        sizeof(float) * m_channelCount * count;
    
    m_d->buffer.mBuffers[0].mData = frames;

    UInt32 framesRead = count;
    UInt32 extractionFlags = 0;

    m_d->err = MovieAudioExtractionFillBuffer
        (m_d->extractionSessionRef, &framesRead, &m_d->buffer, &extractionFlags);
    if (m_d->err) {
        m_error = "OggVorbisReadStream: Error in decoder: code " + codestr(m_d->err);
    }
*/
    return framesRead;
}

std::vector<std::string>
OggVorbisReadStream::getSupportedFileExtensions()
{
    const char *exts[] = {
	"ogg", "oga"
    };
    std::vector<std::string> rv;
    for (int i = 0; i < sizeof(exts)/sizeof(exts[0]); ++i) {
	rv.push_back(exts[i]);
    }
    return rv;
}

}

#endif

