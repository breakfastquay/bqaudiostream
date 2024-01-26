/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/*
    bqaudiostream

    A small library wrapping various audio file read/write
    implementations in C++.

    Copyright 2007-2020 Particular Programs Ltd.

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

#include "SimpleWavFileReadStream.h"

#if ! (defined(HAVE_LIBSNDFILE) || defined(HAVE_SNDFILE))

#include <iostream>

//#define DEBUG_SIMPLE_WAV_FILE_READ_STREAM 1

namespace breakfastquay
{

static std::vector<std::string> extensions() {
    std::vector<std::string> ee;
    ee.push_back("wav");
    return ee;
}

static 
AudioReadStreamBuilder<SimpleWavFileReadStream>
simplewavbuilder(
    std::string("http://breakfastquay.com/rdf/turbot/audiostream/SimpleWavFileReadStream"),
    extensions()
    );

SimpleWavFileReadStream::SimpleWavFileReadStream(std::string filename) :
    m_path(filename),
    m_file(0),
    m_bitDepth(0),
    m_dataChunkSize(0),
    m_dataReadOffset(0),
    m_dataReadStart(0)
{
#ifdef _MSC_VER
    // This is behind _MSC_VER not _WIN32 because the fstream
    // constructors from wchar bufs are an MSVC extension not
    // available in e.g. MinGW
    int wlen = MultiByteToWideChar
        (CP_UTF8, 0, m_path.c_str(), m_path.length(), 0, 0);
    if (wlen > 0) {
        wchar_t *buf = new wchar_t[wlen+1];
        (void)MultiByteToWideChar
            (CP_UTF8, 0, m_path.c_str(), m_path.length(), buf, wlen);
        buf[wlen] = L'\0';
        m_file = new std::ifstream(buf, std::ios::in | std::ios::binary);
        delete[] buf;
    }
#else
    m_file = new std::ifstream(m_path.c_str(), std::ios::in | std::ios::binary);
#endif
    
    if (!*m_file) {
        delete m_file;
        m_file = 0;
        throw FileNotFound(m_path);
    }

    readHeader();
}

SimpleWavFileReadStream::~SimpleWavFileReadStream()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }
}

void
SimpleWavFileReadStream::readHeader()
{
    if (!m_file) {
        throw std::logic_error("internal error: no file in readHeader");
    }
    
    (void) readExpectedChunkSize("RIFF");

    std::string found = readTag();
    if (found != "WAVE") {
        throw InvalidFileFormat
            (m_path, "RIFF file is not WAVE format");
    }

    uint32_t fmtSize = readExpectedChunkSize("fmt ");
    if (fmtSize < 16) {
        std::cout << "fmtSize = " << fmtSize << std::endl;
        throw InvalidFileFormat(m_path, "unexpectedly short format chunk");
    }

    uint32_t audioFormat = readMandatoryNumber(2);
    if (audioFormat != 1 && audioFormat != 3) {
        throw InvalidFileFormat(m_path, "only PCM and float WAV formats are supported by this reader");
    }

    uint32_t channels = readMandatoryNumber(2);
    uint32_t sampleRate = readMandatoryNumber(4);
    uint32_t byteRate = readMandatoryNumber(4);
    uint32_t bytesPerFrame = readMandatoryNumber(2);
    uint32_t bitsPerSample = readMandatoryNumber(2);
    
    if (bitsPerSample != 8 &&
        bitsPerSample != 16 &&
        bitsPerSample != 24 &&
        bitsPerSample != 32) {
        throw InvalidFileFormat(m_path, "unsupported bit depth");
    }

    if (bitsPerSample == 32) {
        if (audioFormat == 1) {
            throw InvalidFileFormat(m_path, "32-bit samples are only supported in float format, not PCM");
        } else {
            char *buf = (char *)malloc(sizeof(float));
            *(float *)buf = -0.f;
            m_floatSwap = (buf[0] != '\0');
        }
    }

    m_channelCount = channels;
    m_sampleRate = sampleRate;
    m_bitDepth = bitsPerSample;
    m_seekable = true;

    // we don't use
    (void)byteRate;

    // and we ignore extended format chunk data
    if (fmtSize > 16) {
        m_file->ignore(fmtSize - 16);
    }

    m_dataChunkSize = readExpectedChunkSize("data");
    if (bytesPerFrame > 0) {
        m_estimatedFrameCount = m_dataChunkSize / bytesPerFrame;
    } else {
        m_estimatedFrameCount = 0;
    }
    m_dataReadOffset = 0;
    m_dataReadStart = m_file->tellg();
}

uint32_t
SimpleWavFileReadStream::readExpectedChunkSize(std::string tag)
{
    // Read up to and including the given tag and its chunk size,
    // skipping any mismatching chunks that precede it. Return the
    // chunk size for the given tag
    
    readExpectedTag(tag);
    return readChunkSizeAfterTag();
}

void
SimpleWavFileReadStream::readExpectedTag(std::string tag)
{
    // Read up to and including the given tag, without reading its
    // following chunk size, skipping any mismatching chunks that
    // precede it
    
    while (true) {
        std::string found = readTag();
        if (found == "") {
            throw InvalidFileFormat
                (m_path, "end-of-file before expected tag \"" + tag + "\"");
        }
        if (found == tag) return;

        // wrong tag: must skip it (or fail)
        uint32_t chunkSize = readChunkSizeAfterTag();
        m_file->ignore(chunkSize);
        if (!m_file->good()) {
            throw InvalidFileFormat
                (m_path, "incomplete chunk following tag \"" + found + "\"");
        }
    }
}

std::string
SimpleWavFileReadStream::readTag()
{
    std::vector<uint8_t> v(4);
    int obtained = getBytes(4, v);
    if (obtained == 0) return "";
    if (obtained != 4) {
        throw InvalidFileFormat(m_path, "incomplete tag");
    }
    std::string tag((const char *)v.data(), 4);
    return tag;
}

uint32_t
SimpleWavFileReadStream::readMandatoryNumber(int length)
{
    std::vector<uint8_t> v(length);
    if (getBytes(length, v) != length) {
        throw InvalidFileFormat(m_path, "incomplete number");
    }
    return le2int(v);
}

uint32_t
SimpleWavFileReadStream::readChunkSizeAfterTag()
{
    return readMandatoryNumber(4);
}

bool
SimpleWavFileReadStream::performSeek(size_t frame)
{
    if (m_file->fail()) {
#ifdef DEBUG_SIMPLE_WAV_FILE_READ_STREAM
        std::cerr << "SimpleWavFileReadStream::performSeek: "
                  << "file is in failure state! failing the seek"
                  << std::endl;
#endif
        return false;
    }
    
    int sampleSize = m_bitDepth / 8;
    int frameSize = sampleSize * m_channelCount;

    std::ifstream::off_type target =
        std::ifstream::off_type(frame * frameSize + m_dataReadStart);

#ifdef DEBUG_SIMPLE_WAV_FILE_READ_STREAM
    std::cerr << "SimpleWavFileReadStream[" << this << "]::performSeek: frame "
              << frame << " with frameSize " << frameSize
              << " and m_dataReadStart " << m_dataReadStart
              << " gives target " << target << std::endl;
#endif
    
    if (m_dataChunkSize > 0) {
        // Known size - m_dataChunkSize *should* be >0 but it isn't
        // always, e.g. when the file is being written as we read
        if (target > m_dataChunkSize + m_dataReadStart) {
#ifdef DEBUG_SIMPLE_WAV_FILE_READ_STREAM
            std::cerr << "SimpleWavFileReadStream::performSeek: seek to "
                      << target << " is beyond data end "
                      << m_dataChunkSize + m_dataReadStart
                      << ", failing the seek" << std::endl;
#endif
            return false;
        }
    }
    
    m_file->seekg(target, std::ios::beg);
    if (m_file->fail()) {
#ifdef DEBUG_SIMPLE_WAV_FILE_READ_STREAM
        std::cerr << "SimpleWavFileReadStream::performSeek: seek to "
                  << target << " failed (failbit set), failing the seek"
                  << std::endl;
#endif
        return false;
    }
    
    std::ifstream::pos_type actual = m_file->tellg();
    // (In fact I think tellg() always reports whatever you passed to seekg())
    if (actual != std::ifstream::pos_type(target)) {
#ifdef DEBUG_SIMPLE_WAV_FILE_READ_STREAM
        std::cerr << "SimpleWavFileReadStream::performSeek: seek to "
                  << target << " brought us to " << actual
                  << ", failing the seek" << std::endl;
#endif
        return false;
    }

    if (actual < std::ifstream::pos_type(m_dataReadStart)) {
#ifdef DEBUG_SIMPLE_WAV_FILE_READ_STREAM
        std::cerr << "SimpleWavFileReadStream::performSeek: seek to "
                  << actual << " lands before data start, failing the seek"
                  << std::endl;
#endif
        return false;
    }
        
    m_dataReadOffset = uint32_t(actual -
                                std::ifstream::pos_type(m_dataReadStart));

#ifdef DEBUG_SIMPLE_WAV_FILE_READ_STREAM
    std::cerr << "SimpleWavFileReadStream::performSeek: successful, new offset "
              << m_dataReadOffset << std::endl;;
#endif
    
    return true;
}

size_t
SimpleWavFileReadStream::getFrames(size_t count, float *frames)
{
    int sampleSize = m_bitDepth / 8;
    std::vector<uint8_t> buf(sampleSize);
    
    size_t requested = count * m_channelCount;
    size_t got = 0;

    while (got < requested) {
        if (m_dataChunkSize > 0 && m_dataReadOffset >= m_dataChunkSize) {
            break;
        }
        int gotHere = getBytes(sampleSize, buf);
        m_dataReadOffset += gotHere;
        if (gotHere < sampleSize) {
            break;
        }
        switch (m_bitDepth) {
        case 8: frames[got] = convertSample8(buf); break;
        case 16: frames[got] = convertSample16(buf); break;
        case 24: frames[got] = convertSample24(buf); break;
        case 32: frames[got] = convertSampleFloat(buf); break;
        }
        ++got;
    }

    if (got < requested) {
#ifdef DEBUG_SIMPLE_WAV_FILE_READ_STREAM
        std::cerr << "SimpleWavFileReadStream::getFrames: EOF reached after "
                  << got << " of " << requested << " samples" << std::endl;
#endif
    }

    if (m_file->eof() && !m_file->bad()) {
        m_file->clear();
    }
    
    return got / m_channelCount;
}

float
SimpleWavFileReadStream::convertSample8(const std::vector<uint8_t> &v)
{
    return float(int32_t(v[0]) - 128) / 128.0;
}

float
SimpleWavFileReadStream::convertSample16(const std::vector<uint8_t> &v)
{
    uint32_t b0 = v[0], b1 = v[1];

    uint32_t bb = ((b0 << 16) | (b1 << 24));
    int32_t i(bb);

    return float(i) / 2147483647.f;
}

float
SimpleWavFileReadStream::convertSample24(const std::vector<uint8_t> &v)
{
    uint32_t b0 = v[0], b1 = v[1], b2 = v[2];

    uint32_t bb = ((b0 << 8) | (b1 << 16) | (b2 << 24));
    int32_t i(bb);

    return float(i) / 2147483647.f;
}

float
SimpleWavFileReadStream::convertSampleFloat(const std::vector<uint8_t> &v)
{
    if (!m_floatSwap) {
        const uint8_t *buf = v.data();
        return *(const float *)buf;
    } else {
        std::vector<uint8_t> vv(4);
        for (int i = 0; i < 4; ++i) {
            vv[i] = v[3-i];
        }
        const uint8_t *buf = vv.data();
        return *(const float *)buf;
    }
}

int
SimpleWavFileReadStream::getBytes(int n, std::vector<uint8_t> &v)
{
    if (!m_file) return 0;

    for (int i = 0; i < n; ++i) {
        char c;
        m_file->get(c);
        if (!m_file->good()) {
            return i;
        } else {
            v[i] = (uint8_t)c;
        }
    }

    return n;
}

uint32_t
SimpleWavFileReadStream::le2int(const std::vector<uint8_t> &le)
{
    uint32_t n = 0;
    int len = int(le.size());

    for (int i = 0; i < len; ++i) {
        n += (uint32_t(le[i]) << (8 * i));
    }

    return n;
}

}

#endif
