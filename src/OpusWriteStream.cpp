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

#ifdef HAVE_OPUS

#include "OpusWriteStream.h"

#include <opus/opusenc.h>

#include <iostream>
#include <sstream>

using namespace std;

namespace breakfastquay
{

static vector<string>
getOpusWriteExtensions()
{
    vector<string> extensions;
    extensions.push_back("opus");
    return extensions;
}

static 
AudioWriteStreamBuilder<OpusWriteStream>
opuswritebuilder(
    string("http://breakfastquay.com/rdf/turbot/audiostream/OpusWriteStream"),
    getOpusWriteExtensions()
    );

class OpusWriteStream::D
{
public:
    D() : encoder(0) { }

    OggOpusComments *comments;
    OggOpusEnc *encoder;
};

OpusWriteStream::OpusWriteStream(Target target) :
    AudioWriteStream(target),
    m_d(new D)
{
//    cerr << "OpusWriteStream: file is " << getPath() << ", channel count is " << getChannelCount() << ", sample rate " << getSampleRate() << endl;

    //!!! +windows file encoding?

    m_d->comments = ope_comments_create();
    
    int err = 0;
    m_d->encoder = ope_encoder_create_file(getPath().c_str(), m_d->comments,
                                           getSampleRate(), getChannelCount(),
                                           getChannelCount() > 2 ? 1 : 0,
                                           &err);

    if (err || !m_d->encoder) {
        ostringstream os;    
        os << "OpusWriteStream: Unable to open file for writing (error code "
           << err << ")";
        m_error = os.str();
        cerr << m_error << endl;
        m_d->encoder = 0;
        throw FailedToWriteFile(getPath());
    }    
}

OpusWriteStream::~OpusWriteStream()
{
    if (m_d->encoder) {
//        cerr << "OpusWriteStream::~OpusWriteStream: closing" << endl;
        int err = ope_encoder_drain(m_d->encoder);
        if (err) {
            cerr << "WARNING: ope_encoder_drain failed (error code "
                 << err << ")" << endl;
        }
        ope_encoder_destroy(m_d->encoder);
        ope_comments_destroy(m_d->comments);
    }
}

void
OpusWriteStream::putInterleavedFrames(size_t count, const float *frames)
{
    if (count == 0 || !m_d->encoder) return;

    int err = ope_encoder_write_float(m_d->encoder, frames, count);

    if (err) {
        ostringstream os;    
        os << "OpusWriteStream: Failed to write frames to encoder (error code "
           << err << ")";
        m_error = os.str();
        cerr << m_error << endl;
        throw FileOperationFailed(getPath(), "encode");
    }
}

}

#endif
