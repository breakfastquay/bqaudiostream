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

#ifdef HAVE_MINIMP3

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include <minimp3_ex.h>

#include "MiniMP3ReadStream.h"

#include <sstream>

namespace breakfastquay
{

static vector<string>
getMiniMP3Extensions()
{
    vector<string> extensions;
    extensions.push_back("mp3");
    return extensions;
}

static
AudioReadStreamBuilder<MiniMP3ReadStream>
minimp3builder(
    string("http://breakfastquay.com/rdf/turbot/audiostream/MiniMP3ReadStream"),
    getMiniMP3Extensions()
    );

class MiniMP3ReadStream::D
{
public:
    D() { }
    mp3dec_ex_t dec;
};

MiniMP3ReadStream::MiniMP3ReadStream(string path) :
    m_path(path),
    m_d(new D)
{
    //!!! Windows: use wchar versions
    
    m_channelCount = 0;
    m_sampleRate = 0;

    int err = mp3dec_ex_open(&m_d->dec, path.c_str(), 0);
    if (err) {
        ostringstream os;
        os << "MiniMP3ReadStream: Unable to open file (error code " << err << ")";
        m_error = os.str();
        if (err == MP3D_E_IOERROR) {
            throw FileNotFound(m_path);
        } else {
            throw InvalidFileFormat(m_path, "failed to open audio file");
        }
    }

    m_channelCount = m_d->dec.info.channels;
    m_sampleRate = m_d->dec.info.hz;
    m_estimatedFrameCount = m_d->dec.samples / m_channelCount;
}

size_t
MiniMP3ReadStream::getFrames(size_t count, float *frames)
{
    if (m_error != "") return 0;
    if (count == 0) return 0;

//    cerr << "getFrames: working" << endl;

    size_t desired = count * m_channelCount;
    size_t obtained = mp3dec_ex_read(&m_d->dec, frames, desired);

    //!!! have to provide our own tags support?
    
    if (obtained < desired) {
        if (m_d->dec.last_error) {
            ostringstream os;
            os << "MiniMP3ReadStream: Failed to read from file (error code "
               << m_d->dec.last_error << ")";
            m_error = os.str();
            mp3dec_ex_close(&m_d->dec);
            throw InvalidFileFormat(m_path, "error in decoder");
        }
    }

    return obtained / m_channelCount;
}

MiniMP3ReadStream::~MiniMP3ReadStream()
{
    if (m_error != "") {
        mp3dec_ex_close(&m_d->dec);
    }
    delete m_d;
}

}

#endif

