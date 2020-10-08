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

using namespace std;

namespace breakfastquay
{

static vector<string> extensions() {
    vector<string> ee;
    ee.push_back("wav");
    return ee;
}

static 
AudioReadStreamBuilder<SimpleWavFileReadStream>
simplewavbuilder(
    string("http://breakfastquay.com/rdf/turbot/audiostream/SimpleWavFileReadStream"),
    extensions()
    );

SimpleWavFileReadStream::SimpleWavFileReadStream(std::string filename) :
    m_path(filename),
    m_channels(0),
    m_rate(0),
    m_bitDepth(0),
    m_startPos(0),
    m_index(0),
    m_file(nullptr)
{
    m_file = new ifstream(filename.c_str(), ios::in | std::ios::binary);

    if (!*m_file) {
        delete m_file;
        m_file = nullptr;
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
    
    uint32_t totalSize = readExpectedChunkSize("RIFF");
    readExpectedTag("WAVE");
    uint32_t fmtSize = readExpectedChunkSize("fmt ");
    if (fmtSize != 16) {
        cout << "fmtSize = " << fmtSize << endl;
        throw InvalidFileFormat(m_path, "unexpected format chunk size");
    }

    uint32_t audioFormat = readMandatoryNumber(2);
    if (audioFormat != 1) {
        throw InvalidFileFormat(m_path, "PCM formats only supported by this reader");
    }

    uint32_t channels = readMandatoryNumber(2);
    uint32_t sampleRate = readMandatoryNumber(4);
    uint32_t byteRate = readMandatoryNumber(4);
    uint32_t bytesPerSample = readMandatoryNumber(2);
    uint32_t bitsPerSample = readMandatoryNumber(2);

    if (bitsPerSample != 8 &&
        bitsPerSample != 16 &&
        bitsPerSample != 24) {
        throw InvalidFileFormat(m_path, "unsupported bit depth");
    }

    m_channels = channels;
    m_rate = sampleRate;
    m_bitDepth = bitsPerSample;

    while (true) {
        string tag = readTag();
        if (tag == "") {
            throw InvalidFileFormat(m_path, "tag expected");
        }
        if (tag == "data") {
            break;
        } else {
            uint32_t chunkSize = readChunkSizeAfterTag();
            m_file->ignore(chunkSize);
            if (!m_file->good()) {
                throw InvalidFileFormat(m_path, "incomplete chunk");
            }
        }
    }

    // we don't use these
    (void)totalSize;
    (void)byteRate;
    (void)bytesPerSample;
}

uint32_t
SimpleWavFileReadStream::readExpectedChunkSize(string tag)
{
    readExpectedTag(tag);
    return readChunkSizeAfterTag();
}

void
SimpleWavFileReadStream::readExpectedTag(string tag)
{
    string actual = readTag();
    if (actual != tag) {
        throw InvalidFileFormat(m_path, "failed to read tag \"" + tag + "\" (found \"" + actual + "\"");
    }
}

string
SimpleWavFileReadStream::readTag()
{
    vector<uint8_t> v(4);
    if (getBytes(4, v) != 4) {
        throw InvalidFileFormat(m_path, "incomplete tag");
    }
    string tag((const char *)v.data(), 4);
    return tag;
}

uint32_t
SimpleWavFileReadStream::readMandatoryNumber(int length)
{
    vector<uint8_t> v(length);
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

size_t
SimpleWavFileReadStream::getFrames(size_t count, float *frames)
{
    size_t got = 0;
    int sampleSize = m_bitDepth / 8;
    vector<uint8_t> buf(sampleSize);

    while (got < count) {
        int gotHere = getBytes(sampleSize, buf);
        if (gotHere < sampleSize) {
            return got;
        }
        switch (m_bitDepth) {
        case 8: frames[got] = convertSample8(buf); break;
        case 16: frames[got] = convertSample16(buf); break;
        case 24: frames[got] = convertSample24(buf); break;
        }
        ++got;
    }

    return got;
}

float
SimpleWavFileReadStream::convertSample8(const vector<uint8_t> &v)
{
    return float(int32_t(v[0]) - 128) / 128.0;
}

float
SimpleWavFileReadStream::convertSample16(const vector<uint8_t> &v)
{
    uint32_t b0 = v[0], b1 = v[1];

    uint32_t b10 = (b0 | (b1 << 8));

    if (b1 & 0x80) { // negative
        b10 |= 0xffff0000;
    }

    return float(b10) / 32768.f;
}

float
SimpleWavFileReadStream::convertSample24(const vector<uint8_t> &v)
{
    uint32_t b0 = v[0], b1 = v[1], b2 = v[2];

    uint32_t b210 = (b0 | (b1 << 8) | (b2 << 16));

    if (b2 & 0x80) { // negative
        b210 |= 0xff000000;
    }

    return float(b210) / 8388608.f;
}

int
SimpleWavFileReadStream::getBytes(int n, vector<uint8_t> &v)
{
    if (!m_file) return {};

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
SimpleWavFileReadStream::le2int(const vector<uint8_t> &le)
{
    uint32_t n = 0;
    int len = int(le.size());

    for (int i = 0; i < len; ++i) {
        n += (uint32_t(le[i]) << (8 * i));
    }

    cout << "got n = " << n << endl;

    return n;
}

}


#endif
