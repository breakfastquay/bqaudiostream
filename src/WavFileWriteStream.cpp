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

#if defined(HAVE_LIBSNDFILE) || defined(HAVE_SNDFILE)

#include "WavFileWriteStream.h"
#include "../bqaudiostream/Exceptions.h"

#include <cstring>

namespace breakfastquay
{

static std::vector<std::string>
getWavWriterExtensions() {
    std::vector<std::string> ee;
    ee.push_back("wav");
    ee.push_back("aiff");
    return ee;
}

static 
AudioWriteStreamBuilder<WavFileWriteStream>
wavbuilder(
    std::string("http://breakfastquay.com/rdf/turbot/audiostream/WavFileWriteStream"),
    getWavWriterExtensions()
    );

size_t
WavFileWriteStream::m_syncBlockSize = 4096;

WavFileWriteStream::WavFileWriteStream(Target target) :
    AudioWriteStream(target),
    m_file(0),
    m_sinceSync(0)
{
    memset(&m_fileInfo, 0, sizeof(SF_INFO));
    m_fileInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    m_fileInfo.channels = getChannelCount();
    m_fileInfo.samplerate = getSampleRate();

    auto path = getPath();
#ifdef _WIN32
    int wlen = MultiByteToWideChar
        (CP_UTF8, 0, path.c_str(), path.length(), 0, 0);
    if (wlen > 0) {
        wchar_t *buf = new wchar_t[wlen+1];
        (void)MultiByteToWideChar
            (CP_UTF8, 0, path.c_str(), path.length(), buf, wlen);
        buf[wlen] = L'\0';
        m_file = sf_wchar_open(buf, SFM_WRITE, &m_fileInfo);
        delete[] buf;
    }
#else
    m_file = sf_open(path.c_str(), SFM_WRITE, &m_fileInfo);
#endif

    if (!m_file) {
        m_error = std::string("Failed to open audio file '") +
            path + "' for writing";
        throw FailedToWriteFile(path);
    }
}

WavFileWriteStream::~WavFileWriteStream()
{
    if (m_file) sf_close(m_file);
}

void
WavFileWriteStream::putInterleavedFrames(size_t count, const float *frames)
{
    if (count == 0) return;

    sf_count_t written = sf_writef_float(m_file, frames, count);

    if (written != sf_count_t(count)) {
        throw FileOperationFailed(getPath(), "write sf data");
    }

    m_sinceSync += count;
    if (m_sinceSync > m_syncBlockSize) {
        flush();
    }
}

void
WavFileWriteStream::flush()
{
    if (m_file) {
        sf_write_sync(m_file);
        m_sinceSync = 0;
    }
}

}

#endif
