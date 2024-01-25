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

#include "SimpleWavFileWriteStream.h"

#if ! (defined(HAVE_LIBSNDFILE) || defined(HAVE_SNDFILE))

#include "../bqaudiostream/Exceptions.h"
#include <iostream>
#include <stdint.h>

using namespace std;

namespace breakfastquay
{

static std::vector<std::string> extensions() {
    std::vector<std::string> ee;
    ee.push_back("wav");
    return ee;
}

static 
AudioWriteStreamBuilder<SimpleWavFileWriteStream>
simplewavbuilder(
    std::string("http://breakfastquay.com/rdf/turbot/audiostream/SimpleWavFileWriteStream"),
    extensions()
    );

SimpleWavFileWriteStream::SimpleWavFileWriteStream(Target target) :
    AudioWriteStream(target),
    m_bitDepth(24),
    m_file(0)
{
    std::string path = getPath();
    
#ifdef _MSC_VER
    // This is behind _MSC_VER not _WIN32 because the fstream
    // constructors from wchar bufs are an MSVC extension not
    // available in e.g. MinGW
    int wlen = MultiByteToWideChar
        (CP_UTF8, 0, path.c_str(), path.length(), 0, 0);
    if (wlen > 0) {
        wchar_t *buf = new wchar_t[wlen+1];
        (void)MultiByteToWideChar
            (CP_UTF8, 0, path.c_str(), path.length(), buf, wlen);
        buf[wlen] = L'\0';
        m_file = new std::ofstream(buf, std::ios::out | std::ios::binary);
        delete[] buf;
    }
#else
    m_file = new std::ofstream(path.c_str(), std::ios::out | std::ios::binary);
#endif

    if (!*m_file) {
        delete m_file;
        m_file = 0;
        m_error = std::string("Failed to open audio file '") +
            path + "' for writing";
        throw FailedToWriteFile(path);
    }

    writeFormatChunk();
}

static
std::string
int2le(uint32_t value, uint32_t length)
{
    std::string r(length, '\0');

    for (uint32_t i = 0; i < length; ++i) {
        r[i] = (uint8_t)(value & 0xff);
        value >>= 8;
    }

    return r;
}

SimpleWavFileWriteStream::~SimpleWavFileWriteStream()
{
    if (!m_file) {
        return;
    }

    m_file->seekp(0, std::ios::end);

    std::streamoff totalSize = m_file->tellp();
    uint32_t effSize = uint32_t(-1);
    if (totalSize < std::streamoff(effSize)) {
        effSize = uint32_t(totalSize);
    }

    // seek to first length position
    m_file->seekp(4, std::ios::beg);

    // write complete file size minus 8 bytes to here
    putBytes(int2le(effSize - 8, 4));

    // reseek from start forward 40
    m_file->seekp(40, std::ios::beg);

    // write the data chunk size to end
    putBytes(int2le(effSize - 44, 4));

    m_file->close();

    delete m_file;
    m_file = 0;
}

void
SimpleWavFileWriteStream::putBytes(const std::string &s)
{
    if (!m_file) return;
    m_file->write(s.data(), s.length());
}

void
SimpleWavFileWriteStream::putBytes(const uint8_t *buffer, size_t n)
{
    if (!m_file) return;
    m_file->write((const char *)buffer, n);
}

void
SimpleWavFileWriteStream::writeFormatChunk()
{
    if (!m_file) return;

    std::string outString;

    outString += "RIFF";
    outString += int2le(0x0, 4);
    outString += "WAVE";
    outString += "fmt ";

    // length
    outString += int2le(0x10, 4);

    // 1 for PCM, 3 for float
    outString += int2le(0x01, 2);

    // channels
    outString += int2le(getChannelCount(), 2);

    // sample rate
    outString += int2le(getSampleRate(), 4);

    // bytes per second
    outString += int2le((m_bitDepth / 8) * getChannelCount() * getSampleRate(), 4);

    // bytes per frame
    outString += int2le((m_bitDepth / 8) * getChannelCount(), 2);

    // bits per sample
    outString += int2le(m_bitDepth, 2);

    outString += "data";
    outString += int2le(0x0, 4);

    putBytes(outString);

    m_file->flush();
}

void
SimpleWavFileWriteStream::putInterleavedFrames(size_t count, const float *frames)
{
    if (count == 0) return;

    for (size_t i = 0; i < count; ++i) {
        for (size_t c = 0; c < getChannelCount(); ++c) {
            
            double f = frames[i * getChannelCount() + c];
            uint32_t u = 0;
            uint8_t ubuf[4];
            if (f < -1.0) f = -1.0;
            if (f > 1.0) f = 1.0;
            
            switch (m_bitDepth) {

            case 24:
                f = f * 2147483647.0;
                u = (uint32_t)(int(f));
                u >>= 8;
                ubuf[0] = (u & 0xff);
                u >>= 8;
                ubuf[1] = (u & 0xff);
                u >>= 8;
                ubuf[2] = (u & 0xff);
                break;

            default:
                ubuf[0] = ubuf[1] = ubuf[2] = ubuf[3] = '\0';
                break;
            }

            putBytes(ubuf, m_bitDepth / 8);
        }
    }

    m_file->flush();
}

}

#endif
