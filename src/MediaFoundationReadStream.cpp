/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_MEDIAFOUNDATION

#define WINVER 0x0601 // _WIN32_WINNT_WIN7, earliest version to define MF API

#include "MediaFoundationReadStream.h"
#include "base/RingBuffer.h"
#include "system/Thread.h"

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <stdio.h>
#include <mferror.h>

#include <iostream>

#include <QUrl>

using std::cerr;
using std::endl;

namespace Turbot
{

static
AudioReadStreamBuilder<MediaFoundationReadStream>
directshowbuilder(
    QString("http://breakfastquay.com/rdf/turbot/audiostream/MediaFoundationReadStream"),
    QStringList() << "wav" << "mp3" << "wma"
    );


static const int maxBufferSize = 1048575;

class MediaFoundationReadStream::D
{
public:
    D(MediaFoundationReadStream *s) :
        stream(s),
        refCount(0),
        channelCount(0),
        bitDepth(0),
        sampleRate(0),
        isFloat(false),
        reader(0),
        mediaType(0),
        mediaBuffer(0),
        mediaBufferIndex(0),
        err(S_OK),
        complete(false)
        { }

    ~D() {
        if (mediaBuffer) {
            mediaBuffer->Release();
        }

        if (reader) {
            reader->Release();
        }

        if (mediaType) {
            mediaType->Release();
        }  
    }

    ULONG APIENTRY AddRef() {
        return ++refCount;
    }
    
    ULONG APIENTRY Release() {
        return --refCount;
    }

    ULONG refCount;

    MediaFoundationReadStream *stream;

    int channelCount;
    int bitDepth;
    int sampleRate;
    bool isFloat;

    IMFSourceReader *reader;
    IMFMediaType *mediaType;
    IMFMediaBuffer *mediaBuffer;
    int mediaBufferIndex;

    HRESULT err;

    bool complete;

    int getFrames(int count, float *frames);
    void convertSamples(const unsigned char *in, int inbufsize, float *out);
    float convertSample(const unsigned char *);
    void fillBuffer();
};


MediaFoundationReadStream::MediaFoundationReadStream(QString path) :
    m_path(path),
    m_d(new D(this))
{
    m_channelCount = 0;
    m_sampleRate = 0;

    // Reference: 
    // http://msdn.microsoft.com/en-gb/library/windows/desktop/dd757929%28v=vs.85%29.aspx

    cerr << "MediaFoundationReadStream(" << path << ")" << endl;

    if (!QFile(m_path).exists()) throw FileNotFound(m_path);
    
    // Note: CoInitializeEx(NULL, COINIT_MULTITHREADED) must
    // presumably already have been called
    //!!! nb MF docs say COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE

    //!!! do this only once?
    m_d->err = MFStartup(MF_VERSION);
    if (FAILED(m_d->err)) {
        m_error = "MediaFoundationReadStream: Failed to initialise MediaFoundation";
        throw FileOperationFailed(m_path, "MediaFoundation initialise");
    }

    // We are not expected to handle non-local URLs here, I think --
    // convert to file:// URL so as to avoid confusion with path
    // separators & format
    QString url = m_path;
    if (!m_path.startsWith("file:") && !m_path.startsWith("FILE:")) {
        url = QUrl::fromLocalFile(QFileInfo(m_path).absoluteFilePath()).toString();
    }

    cerr << "url = <" << url << ">" << endl;

    WCHAR wpath[MAX_PATH+1];
    MultiByteToWideChar(CP_ACP, 0, url.toLocal8Bit().data(), -1, wpath, MAX_PATH);

    //!!! URL or path?

    m_d->err = MFCreateSourceReaderFromURL(wpath, NULL, &m_d->reader);
    if (FAILED(m_d->err)) {
        m_error = "MediaFoundationReadStream: Failed to create source reader";
        throw FileOperationFailed(m_path, "MediaFoundation create source reader");
    }

    m_d->err = m_d->reader->SetStreamSelection
        ((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    if (SUCCEEDED(m_d->err)) {
        m_d->err = m_d->reader->SetStreamSelection
            ((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
    }
    if (FAILED(m_d->err)) {
        m_error = "MediaFoundationReadStream: Failed to select first audio stream";
        throw FileOperationFailed(m_path, "MediaFoundation stream selection");
    }

    // Create a partial media type that specifies uncompressed PCM audio

    IMFMediaType *partialType = 0;
    m_d->err = MFCreateMediaType(&partialType);
    if (SUCCEEDED(m_d->err)) {
        m_d->err = partialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    }
    if (SUCCEEDED(m_d->err)) {
        m_d->err = partialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    }
    if (SUCCEEDED(m_d->err)) {
        m_d->err = m_d->reader->SetCurrentMediaType
            ((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, partialType);
    }
    if (SUCCEEDED(m_d->err)) {
        m_d->err = m_d->reader->GetCurrentMediaType
            ((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &m_d->mediaType);
    }
    if (SUCCEEDED(m_d->err)) {
        // surely this is redundant, as we did it already? but they do
        // it twice in the example... well, presumably no harm anyway
        m_d->err = m_d->reader->SetStreamSelection
            ((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
    }
    if (partialType) {
        partialType->Release();
        partialType = 0;
    }

    if (SUCCEEDED(m_d->err)) {
        UINT32 depth;
        m_d->err = m_d->mediaType->GetUINT32
            (MF_MT_AUDIO_BITS_PER_SAMPLE, &depth);
        m_d->bitDepth = depth;
    }

    if (SUCCEEDED(m_d->err)) {
        UINT32 rate;
        m_d->err = m_d->mediaType->GetUINT32
            (MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
        m_sampleRate = m_d->sampleRate = rate;
    }

    if (SUCCEEDED(m_d->err)) {
        UINT32 chans;
        m_d->err = m_d->mediaType->GetUINT32
            (MF_MT_AUDIO_NUM_CHANNELS, &chans);
        m_channelCount = m_d->channelCount = chans;
    }

    if (FAILED(m_d->err)) {
        m_error = "MediaFoundationReadStream: File format could not be converted to PCM stream";
        throw FileOperationFailed(m_path, "MediaFoundation media type selection");
    }
}

size_t
MediaFoundationReadStream::getFrames(size_t count, float *frames)
{
//    cerr << "MediaFoundationReadStream::getFrames(" << count << ")" << endl;
    size_t s = m_d->getFrames(count, frames);
/*
    float rms = 0.f;
    for (size_t i = 0; i < s * m_channelCount; ++i) {
        float v = frames[i];
        rms += v*v;
    }
    rms /= s * m_channelCount;
    rms = sqrtf(rms);
    cerr << "rms = " << rms << endl;
*/
    return s;
}

int
MediaFoundationReadStream::D::getFrames(int framesRequired, float *frames)
{
//    cerr << "D::getFrames(" << framesRequired << ")" << endl;

    if (!mediaBuffer) {
        fillBuffer();
        if (!mediaBuffer) {
            // legitimate end of file
            return 0;
        }
    }
        
    BYTE *data = 0;
    DWORD length = 0;

    err = mediaBuffer->Lock(&data, 0, &length);

    if (FAILED(err)) {
        stream->m_error = "MediaFoundationReadStream: Failed to lock media buffer?!";
        throw FileOperationFailed(stream->m_path, "Read from audio file");
    }

    int bytesPerFrame = channelCount * (bitDepth / 8);
    int framesAvailable = (length - mediaBufferIndex) / bytesPerFrame;
    int framesToGet = std::min(framesRequired, framesAvailable);

    if (framesToGet > 0) {
        // have something in the buffer, not necessarily all we need
        convertSamples(data + mediaBufferIndex,
                       framesToGet * bytesPerFrame,
                       frames);
        mediaBufferIndex += framesToGet * bytesPerFrame;
    }

    mediaBuffer->Unlock();
    
    if (framesToGet == framesRequired) {
        // we got enough! rah
        return framesToGet;
    }
    
    // otherwise, we ran out: this buffer has nothing left, release it
    // and call again for the next part

    mediaBuffer->Release();
    mediaBuffer = 0;
    mediaBufferIndex = 0;
    
    return framesToGet +
        getFrames(framesRequired - framesToGet, 
                  frames + framesToGet * channelCount);
}

void
MediaFoundationReadStream::D::fillBuffer()
{
    // assumes mediaBuffer is currently null
    
    DWORD flags = 0;
    IMFSample *sample = 0;

    while (!sample) {

        // "In some cases a call to ReadSample does not generate data,
        // in which case the IMFSample pointer is NULL" (hence the loop)

        err = reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                 0, 0, &flags, 0, &sample);

        if (FAILED(err)) {
            stream->m_error = "MediaFoundationReadStream: Failed to read sample from stream";
            throw FileOperationFailed(stream->m_path, "Read from audio file");
        }

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            return;
        }
    }

    err = sample->ConvertToContiguousBuffer(&mediaBuffer);
    if (FAILED(err)) {
        stream->m_error = "MediaFoundationReadStream: Failed to convert sample to buffer";
        throw FileOperationFailed(stream->m_path, "Read from audio file");
    }
}

void
MediaFoundationReadStream::D::convertSamples(const unsigned char *inbuf,
                                             int inbufbytes,
                                             float *out)
{
    int inix = 0;
    int bytesPerSample = bitDepth / 8;
    while (inix < inbufbytes) {
        *out = convertSample(inbuf + inix);
        out += 1;
        inix += bytesPerSample;
    }
}

float
MediaFoundationReadStream::D::convertSample(const unsigned char *c)
{
    if (isFloat) {
        return *(float *)c;
    }

    switch (bitDepth) {

    case 8: {
            // WAV stores 8-bit samples unsigned, other sizes signed.
            return (float)(c[0] - 128.0) / 128.0;
        }

    case 16: {
            // Two's complement little-endian 16-bit integer.
            // We convert endianness (if necessary) but assume 16-bit short.
            unsigned char b2 = c[0];
            unsigned char b1 = c[1];
            unsigned int bits = (b1 << 8) + b2;
            return float(double(short(bits)) / 32768.0);
        }

    case 24: {
            // Two's complement little-endian 24-bit integer.
            // Again, convert endianness but assume 32-bit int.
            unsigned char b3 = c[0];
            unsigned char b2 = c[1];
            unsigned char b1 = c[2];
            // Rotate 8 bits too far in order to get the sign bit
            // in the right place; this gives us a 32-bit value,
            // hence the larger float divisor
            unsigned int bits = (b1 << 24) + (b2 << 16) + (b3 << 8);
            return float(double(int(bits)) / 2147483648.0);
        }

    default:
        return 0.0f;
    }
}

MediaFoundationReadStream::~MediaFoundationReadStream()
{
    cerr << "MediaFoundationReadStream::~MediaFoundationReadStream" << endl;
    delete m_d;
}

}

#endif

