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

#ifndef BQ_SIMPLE_WAV_FILE_READ_STREAM_H
#define BQ_SIMPLE_WAV_FILE_READ_STREAM_H

#include "AudioReadStream.h"

// If we have libsndfile, we shouldn't be using this class
#if ! (defined(HAVE_LIBSNDFILE) || defined(HAVE_SNDFILE))

#include <fstream>
#include <string>
#include <vector>

namespace breakfastquay
{

class SimpleWavFileReadStream : public AudioReadStream
{
public:
    SimpleWavFileReadStream(std::string path);
    virtual ~SimpleWavFileReadStream();

    virtual std::string getTrackName() const { return m_track; }
    virtual std::string getArtistName() const { return m_artist; }

    virtual std::string getError() const { return m_error; }

protected:
    virtual size_t getFrames(size_t count, float *frames);
    
private:
    std::string m_path;
    std::string m_error;
    std::string m_track;
    std::string m_artist;
    
    int m_channels;
    int m_rate;
    int m_bitDepth;
    bool m_floatSwap;
    size_t m_startPos;
    size_t m_index;

    std::ifstream *m_file;

    void readHeader();
    uint32_t readExpectedChunkSize(std::string tag);
    void readExpectedTag(std::string tag);
    std::string readTag();
    uint32_t readChunkSizeAfterTag();
    uint32_t readMandatoryNumber(int length);

    float convertSample8(const vector<uint8_t> &);
    float convertSample16(const vector<uint8_t> &);
    float convertSample24(const vector<uint8_t> &);
    float convertSampleFloat(const vector<uint8_t> &);

    int getBytes(int n, std::vector<uint8_t> &);
    static uint32_t le2int(const std::vector<uint8_t> &le);
};

}

#endif

#endif

