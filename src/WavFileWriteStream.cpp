/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "WavFileWriteStream.h"

#ifdef HAVE_LIBSNDFILE

#include <iostream>

#include <cstring>

namespace Turbot
{

WavFileWriteStream::WavFileWriteStream(std::string path,
				       size_t channelCount,
				       size_t sampleRate) :
    AudioWriteStream(channelCount, sampleRate),
    m_file(0),
    m_path(path)
{
    memset(&m_fileInfo, 0, sizeof(SF_INFO));
    m_fileInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    m_fileInfo.channels = m_channelCount;
    m_fileInfo.samplerate = m_sampleRate;

    m_file = sf_open(m_path.c_str(), SFM_WRITE, &m_fileInfo);

    if (!m_file) {
	std::cerr << "WavFileWriteStream::initialize: Failed to open output file for writing ("
		  << sf_strerror(m_file) << ")" << std::endl;

        m_error = std::string("Failed to open audio file '") +
            m_path + "' for writing";
        m_channelCount = 0;
	return;
    }
}

WavFileWriteStream::~WavFileWriteStream()
{
    if (m_file) sf_close(m_file);
}

bool
WavFileWriteStream::putInterleavedFrames(size_t count, float *frames)
{
    if (!m_file || !m_channelCount) return false;
    if (count == 0) return false;

    sf_count_t written = sf_writef_float(m_file, frames, count);

    return (written == count);
}

}

#endif
