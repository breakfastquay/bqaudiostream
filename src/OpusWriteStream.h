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

#ifndef BQ_OPUS_WRITE_STREAM_H
#define BQ_OPUS_WRITE_STREAM_H

#include "../bqaudiostream/AudioWriteStream.h"

#ifdef HAVE_OPUS
#ifndef HAVE_OPUS_READ_ONLY

namespace breakfastquay
{
    
class OpusWriteStream : public AudioWriteStream
{
public:
    OpusWriteStream(Target target);
    virtual ~OpusWriteStream();

    virtual std::string getError() const { return m_error; }

    virtual void putInterleavedFrames(size_t count, const float *frames);
    virtual void flush();
    
protected:
    std::string m_error;

    class D;
    D *m_d;
};

}

#endif // !HAVE_OPUS_READ_ONLY
#endif // HAVE_OPUS

#endif
