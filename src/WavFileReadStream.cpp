/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_LIBSNDFILE

#include "WavFileReadStream.h"
#include "Exceptions.h"

#include <iostream>

using namespace std;

namespace breakfastquay
{

static vector<string>
getSupportedExtensions()
{
    vector<string> extensions;
    int count;
    
    if (sf_command(0, SFC_GET_FORMAT_MAJOR_COUNT, &count, sizeof(count))) {
        extensions.push_back("wav");
        extensions.push_back("aiff");
        extensions.push_back("aifc");
        extensions.push_back("aif");
        return extensions;
    }

    SF_FORMAT_INFO info;
    for (int i = 0; i < count; ++i) {
        info.format = i;
        if (!sf_command(0, SFC_GET_FORMAT_MAJOR, &info, sizeof(info))) {
            extensions.push_back(string(info.extension));
        }
    }

    return extensions;
}

static
AudioReadStreamBuilder<WavFileReadStream>
wavbuilder(
    string("http://breakfastquay.com/rdf/turbot/audiostream/WavFileReadStream"),
    getSupportedExtensions()
    );

WavFileReadStream::WavFileReadStream(string path) :
    m_file(0),
    m_path(path),
    m_offset(0)
{
    m_channelCount = 0;
    m_sampleRate = 0;

    m_fileInfo.format = 0;
    m_fileInfo.frames = 0;
    m_file = sf_open(m_path.c_str(), SFM_READ, &m_fileInfo);

    if (!m_file || m_fileInfo.frames <= 0 || m_fileInfo.channels <= 0) {
	cerr << "WavFileReadStream::initialize: Failed to open file \""
                  << path << "\" (" << sf_strerror(m_file) << ")" << endl;
	if (m_file) {
	    m_error = string("Couldn't load audio file '") +
                m_path + "':\n" + sf_strerror(m_file);
	} else {
	    m_error = string("Failed to open audio file '") +
		m_path + "'";
	}
        throw InvalidFileFormat(m_path, m_error);
    }

    m_channelCount = m_fileInfo.channels;
    m_sampleRate = m_fileInfo.samplerate;

    sf_seek(m_file, 0, SEEK_SET);
}

WavFileReadStream::~WavFileReadStream()
{
    if (m_file) sf_close(m_file);
}

size_t
WavFileReadStream::getFrames(size_t count, float *frames)
{
    if (!m_file || !m_channelCount) return 0;
    if (count == 0) return 0;

    if ((long)m_offset >= m_fileInfo.frames) {
	return 0;
    }

    sf_count_t readCount = 0;

    if ((readCount = sf_readf_float(m_file, frames, count)) < 0) {
        return 0;
    }

    m_offset = m_offset + readCount;

    return readCount;
}

}

#endif
