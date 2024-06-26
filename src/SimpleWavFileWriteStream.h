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

#ifndef BQ_SIMPLE_WAV_FILE_WRITE_STREAM_H
#define BQ_SIMPLE_WAV_FILE_WRITE_STREAM_H

#include "../bqaudiostream/AudioWriteStream.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <fstream>
#include <string>

namespace breakfastquay
{
    
class SimpleWavFileWriteStream : public AudioWriteStream
{
public:
    SimpleWavFileWriteStream(Target target);
    virtual ~SimpleWavFileWriteStream();

    virtual std::string getError() const { return m_error; }

    virtual void putInterleavedFrames(size_t count, const float *frames);
    virtual void flush();
    
protected:
    int m_bitDepth;
    std::string m_error;
    std::ofstream *m_file;
    size_t m_sinceSync;
    static size_t m_syncBlockSize;

    void writeFormatChunk();
    void putBytes(const std::string &);
    void putBytes(const unsigned char *, size_t);
};

}

#endif

