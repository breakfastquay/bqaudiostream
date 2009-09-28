/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "WavFileReadStream.h"

#include <iostream>

#ifdef HAVE_LIBSNDFILE

namespace Turbot
{

static AudioFileReadStreamBuilder<WavFileReadStream>
builder(WavFileReadStream::getUri());

QUrl
WavFileReadStream::getUri()
{
    return QUrl("http://breakfastquay.com/rdf/turbot/fileio/WavFileReadStream");
}

WavFileReadStream::WavFileReadStream(std::string path) :
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
	std::cerr << "WavFileReadStream::initialize: Failed to open file \""
                  << path << "\" ("
		  << sf_strerror(m_file) << ")" << std::endl;

	if (m_file) {
	    m_error = std::string("Couldn't load audio file '") +
                m_path + "':\n" + sf_strerror(m_file);
	} else {
	    m_error = std::string("Failed to open audio file '") +
		m_path + "'";
	}
	return;
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
WavFileReadStream::getInterleavedFrames(size_t count, float *frames)
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

std::vector<std::string>
WavFileReadStream::getSupportedFileExtensions()
{
    std::vector<std::string> extensions;
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
            extensions.push_back(info.extension);
        }
    }

    return extensions;
}

}

#endif
