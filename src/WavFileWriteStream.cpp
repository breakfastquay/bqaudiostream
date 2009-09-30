/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_LIBSNDFILE

#include "WavFileWriteStream.h"

#include "system/Debug.h"

#include <cstring>

namespace Turbot
{

static 
AudioWriteStreamBuilder<WavFileWriteStream>
wavbuilder(
    QUrl("http://breakfastquay.com/rdf/turbot/fileio/WavFileWriteStream"),
    QStringList() << "wav" << "aiff"
    );

WavFileWriteStream::WavFileWriteStream(Target target) :
    AudioWriteStream(target),
    m_file(0)
{
    memset(&m_fileInfo, 0, sizeof(SF_INFO));
    m_fileInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    m_fileInfo.channels = getChannelCount();
    m_fileInfo.samplerate = getSampleRate();

    m_file = sf_open(getPath().toLocal8Bit().data(), SFM_WRITE, &m_fileInfo);

    if (!m_file) {
	std::cerr << "WavFileWriteStream::initialize: Failed to open output file for writing ("
		  << sf_strerror(m_file) << ")" << std::endl;

        m_error = QString("Failed to open audio file '") +
            getPath() + "' for writing";
        m_target.invalidate();
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
    if (!m_file || !getChannelCount()) return false;
    if (count == 0) return false;

    sf_count_t written = sf_writef_float(m_file, frames, count);

    return (written == count);
}

}

#endif
