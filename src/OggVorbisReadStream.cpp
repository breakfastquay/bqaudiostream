/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/*
    bqaudiostream

    A small library wrapping various audio file read/write
    implementations in C++.

    Copyright 2007-2022 Particular Programs Ltd.

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the names of Chris Cannam and
    Particular Programs Ltd shall not be used in advertising or
    otherwise to promote the sale, use or other dealings in this
    Software without prior written authorization.
*/

#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND

#include "OggVorbisReadStream.h"

#include <bqvec/RingBuffer.h>

#include <oggz/oggz.h>
#include <fishsound/fishsound.h>

namespace breakfastquay
{

static std::vector<std::string>
getOggExtensions()
{
    std::vector<std::string> extensions;
    extensions.push_back("ogg");
    extensions.push_back("oga");
    return extensions;
}

static
AudioReadStreamBuilder<OggVorbisReadStream>
oggbuilder(
    std::string("http://breakfastquay.com/rdf/turbot/audiostream/OggVorbisReadStream"),
    getOggExtensions()
    );

class OggVorbisReadStream::D
{
public:
    D(OggVorbisReadStream *rs) :
        m_rs(rs),
        m_oggz(0),
        m_fishSound(0),
        m_buffer(0),
        m_namesRead(false),
        m_finished(false) { }
    ~D() {
	if (m_fishSound) fish_sound_delete(m_fishSound);
	if (m_oggz) oggz_close(m_oggz);
        delete m_buffer;
    }

    OggVorbisReadStream *m_rs;
    OGGZ *m_oggz;
    FishSound *m_fishSound;
    RingBuffer<float> *m_buffer;
    bool m_namesRead;
    bool m_finished;

    std::string m_trackName;
    std::string m_artistName;

    bool isFinished() const {
        return m_finished;
    }

    std::string getTrackName() const {
        return m_trackName;
    }

    std::string getArtistName() const {
        return m_artistName;
    }

    int getAvailableFrameCount() const {
        if (!m_buffer) return 0;
        return m_buffer->getReadSpace() / int(m_rs->getChannelCount());
    }

    void readNextBlock() {
        if (m_finished) return;
        static const int blocksize = 8192;
        long rv = oggz_read(m_oggz, blocksize);
//        fprintf(stderr, "ogg: read=%ld\n", rv);
        if (rv <= 0) {
            m_finished = true;
        }
    }

    void sizeBuffer(int minFrames) {
        int samples = minFrames * int(m_rs->getChannelCount());
        if (!m_buffer) {
            m_buffer = new RingBuffer<float>(samples);
        } else if (m_buffer->getSize() < samples) {
            RingBuffer<float> *oldBuffer = m_buffer;
            m_buffer = oldBuffer->resized(samples);
            delete oldBuffer;
        }
    }

    int acceptPacket(ogg_packet *p) {
        fish_sound_prepare_truncation
            (m_fishSound, p->granulepos, int(p->e_o_s));
        fish_sound_decode(m_fishSound, p->packet, p->bytes);
        return 0;
    }

    int acceptFrames(float **frames, long n) {

        if (n <= 0) {
            return 0;
        }
        
        if (!m_namesRead) {
            const FishSoundComment *c;
            c = fish_sound_comment_first_byname(m_fishSound, (char *)"TITLE");
            if (c && c->value) {
                m_trackName = std::string(c->value);
            }
            c = fish_sound_comment_first_byname(m_fishSound, (char *)"ARTIST");
            if (c && c->value) {
                m_artistName = std::string(c->value);
            }
            m_namesRead = true;
        }
        
        if (m_rs->getChannelCount() == 0) {
            FishSoundInfo fsinfo;
            fish_sound_command(m_fishSound, FISH_SOUND_GET_INFO,
                               &fsinfo, sizeof(FishSoundInfo));
            m_rs->m_channelCount = fsinfo.channels;
            m_rs->m_sampleRate = fsinfo.samplerate;
        }

        sizeBuffer(getAvailableFrameCount() + int(n));
        int channels = int(m_rs->getChannelCount());
        float *interleaved = allocate<float>(n * channels);
        v_interleave(interleaved, frames, channels, int(n));
        m_buffer->write(interleaved, int(n) * channels);
        deallocate(interleaved);
        return 0;
    }

    static int acceptPacketStatic(OGGZ *, ogg_packet *packet, long, void *data) {
        D *d = (D *)data;
        return d->acceptPacket(packet);
    }

    static int acceptFramesStatic(FishSound *, float **frames, long n, void *data) {
        D *d = (D *)data;
        return d->acceptFrames(frames, n);
    }
};

OggVorbisReadStream::OggVorbisReadStream(std::string path) :
    m_path(path),
    m_d(new D(this))
{
    m_channelCount = 0;
    m_sampleRate = 0;

    //!!! todo: accommodate Windows UTF16
    
    if (!(m_d->m_oggz = oggz_open(m_path.c_str(), OGGZ_READ))) {
	m_error = std::string("File \"") + m_path + "\" is not an Ogg file.";
        throw InvalidFileFormat(m_path, m_error);
    }

    FishSoundInfo fsinfo;
    m_d->m_fishSound = fish_sound_new(FISH_SOUND_DECODE, &fsinfo);
    
    fish_sound_set_decoded_callback(m_d->m_fishSound, D::acceptFramesStatic, m_d);
    oggz_set_read_callback
        (m_d->m_oggz, -1, (OggzReadPacket)D::acceptPacketStatic, m_d);

    // initialise m_channelCount
    while (m_channelCount == 0 && !m_d->m_finished) {
        m_d->readNextBlock(); 
    }

    if (m_channelCount == 0) {
	m_error = std::string("File \"") + m_path + "\" is not a valid Ogg Vorbis file.";
        throw InvalidFileFormat(m_path, m_error);
    }
}

OggVorbisReadStream::~OggVorbisReadStream()
{
    delete m_d;
}

std::string
OggVorbisReadStream::getTrackName() const
{
    return m_d->getTrackName();
}

std::string
OggVorbisReadStream::getArtistName() const
{
    return m_d->getArtistName();
}

size_t
OggVorbisReadStream::getFrames(size_t count, float *frames)
{
    if (!m_channelCount) return 0;
    if (count == 0) return 0;

    //!!! handle sizes > INT_MAX throughout...
    
//    fprintf(stderr, "ogg: getFrames(%d)\n", int(count));

    int available = 0;
    int total = 0;
    
    while (size_t(available = m_d->getAvailableFrameCount()) < count) {
        if (m_d->isFinished()) break;
        int obtained = m_d->m_buffer->read(frames, available * m_channelCount);
        frames += obtained;
        total += obtained;
        count -= available;
        m_d->readNextBlock();
    }

    available = m_d->getAvailableFrameCount();

//    fprintf(stderr, "ogg: requested=%d, available=%d\n", int(count), available);
    
    if (size_t(available) < count) {
        count = available;
    }

    total += m_d->m_buffer->read(frames, int(count * m_channelCount));
    return total / m_channelCount;
}

}

#endif
#endif
