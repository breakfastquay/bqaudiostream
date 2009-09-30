/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "SimpleWavFileWriteStream.h"

#include <iostream>

using std::cerr;
using std::endl;

namespace Turbot
{

SimpleWavFileWriteStream::SimpleWavFileWriteStream(QString path,
                                                   size_t channelCount,
                                                   size_t sampleRate) :
    AudioWriteStream(channelCount, sampleRate),
    m_bitDepth(24),
    m_path(path),
    m_file(0)
{
    m_file = new std::ofstream(m_path.toLocal8Bit().data(),
                               std::ios::out | std::ios::binary);

    if (!*m_file) {
        delete m_file;
        m_file = 0;
        cerr << "SimpleWavFileWriteStream: Failed to open output file for writing" << endl;
        m_error = QString("Failed to open audio file '") +
            m_path + "' for writing";
        m_channelCount = 0;
        return;
    }

    writeFormatChunk();
}

SimpleWavFileWriteStream::~SimpleWavFileWriteStream()
{
    if (!m_file) {
        return;
    }

    m_file->seekp(0, std::ios::end);
    unsigned int totalSize = m_file->tellp();

    // seek to first length position
    m_file->seekp(4, std::ios::beg);

    // write complete file size minus 8 bytes to here
    putBytes(int2le(totalSize - 8, 4));

    // reseek from start forward 40
    m_file->seekp(40, std::ios::beg);

    // write the data chunk size to end
    putBytes(int2le(totalSize - 44, 4));

    m_file->close();

    delete m_file;
    m_file = 0;
}

void
SimpleWavFileWriteStream::putBytes(std::string s)
{
    if (!m_file) return;
    for (unsigned int i = 0; i < s.length(); i++) {
        *m_file << (unsigned char)s[i];
    }
}

void
SimpleWavFileWriteStream::putBytes(const unsigned char *buffer, size_t n)
{
    if (!m_file) return;
    m_file->write((const char *)buffer, n);
}

std::string
SimpleWavFileWriteStream::int2le(unsigned int value, unsigned int length)
{
    std::string r;

    do {
        r += (unsigned char)((long)((value >> (8 * r.length())) & 0xff));
    } while (r.length() < length);

    return r;
}

void
SimpleWavFileWriteStream::writeFormatChunk()
{
    if (!m_file) return;

    std::string outString;

    outString += "RIFF";
    outString += "0000";
    outString += "WAVE";
    outString += "fmt ";

    // length
    outString += int2le(0x10, 4);

    // 1 for PCM, 3 for float
    outString += int2le(0x01, 2);

    // channels
    outString += int2le(m_channelCount, 2);

    // sample rate
    outString += int2le(m_sampleRate, 4);

    // bytes per second
    outString += int2le((m_bitDepth / 8) * m_channelCount * m_sampleRate, 4);

    // bytes per frame
    outString += int2le((m_bitDepth / 8) * m_channelCount, 2);

    // bits per sample
    outString += int2le(m_bitDepth, 2);

    outString += "data";
    outString += "0000";

    putBytes(outString);
}

bool
SimpleWavFileWriteStream::putInterleavedFrames(size_t count, float *frames)
{
    if (!m_file || !m_channelCount) return false;
    if (count == 0) return false;

    for (size_t i = 0; i < count; ++i) {
        for (size_t c = 0; c < m_channelCount; ++c) {
            
            double f = frames[i * m_channelCount + c];
            unsigned int u = 0;
            unsigned char ubuf[4];
            if (f < -1.0) f = -1.0;
            if (f > 1.0) f = 1.0;
            
            switch (m_bitDepth) {

            case 24:
                f = f * 2147483647.0;
                u = (unsigned int)(int(f));
                u >>= 8;
                ubuf[0] = (u & 0xff);
                u >>= 8;
                ubuf[1] = (u & 0xff);
                u >>= 8;
                ubuf[2] = (u & 0xff);
                break;

            default:
                ubuf[0] = ubuf[1] = ubuf[2] = ubuf[3] = '\0';
                break;
            }

            putBytes(ubuf, m_bitDepth / 8);
        }
    }
                
    return true;
}

}

