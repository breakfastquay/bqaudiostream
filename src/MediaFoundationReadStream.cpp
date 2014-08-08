/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_MEDIAFOUNDATION

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

class MediaFoundationReadStream::D : public ISampleGrabberCB
{
public:
    D() :
        refCount(0),
        channelCount(0),
        bitDepth(0),
        sampleRate(0),
        isFloat(false),
        rdbuf(0),
        rdbufsiz(0),
        cvbuf(0),
        cvbufsiz(0),
        reader(0),
        mediaType(0),
        err(S_OK),
        complete(false),
        buffer(0),
        dataAvailable("MediaFoundationReadStream::dataAvailable"),
        spaceAvailable("MediaFoundationReadStream::spaceAvailable")
        { }

    ~D() {
        deallocate<char>(rdbuf);
        deallocate<float>(cvbuf);
        delete buffer;
    }

    ULONG APIENTRY AddRef() {
        return ++refCount;
    }
    
    ULONG APIENTRY Release() {
        return --refCount;
    }

    ULONG refCount;

    size_t channelCount;
    size_t bitDepth;
    size_t sampleRate;
    bool isFloat;

    char *rdbuf;
    int rdbufsiz;
    
    float *cvbuf;
    int cvbufsiz;

    IMFSourceReader *reader;
    IMFMediaType *mediaType;

    HRESULT err;

    bool complete;

    RingBuffer<float> *buffer;
    Mutex mutex;
    Condition dataAvailable;
    Condition spaceAvailable;

    float convertSample(const unsigned char *);
    void fillBuffer();
};


MediaFoundationReadStream::MediaFoundationReadStream(QString path) :
    m_path(path),
    m_d(new D)
{
    m_channelCount = 0;
    m_sampleRate = 0;

    // Reference: 
    // http://msdn.microsoft.com/en-gb/library/windows/desktop/dd757929%28v=vs.85%29.aspx

    cerr << "MediaFoundationReadStream(" << path << ")" << endl;

    if (!QFile(m_path).exists()) throw FileNotFound(m_path);

    HANDLE hFile = INVALID_HANDLE_VALUE;

    // Note: CoInitializeEx(NULL, COINIT_MULTITHREADED) must
    // presumably already have been called
    //!!! nb MF docs say COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE

    //!!! do this only once?
    m_d->err = MFStartup(MF_VERSION);
    if (FAILED(m_d->err)) {
        m_error = "MediaFoundationReadStream: Failed to initialise MediaFoundation";
        throw FileOperationFailed(m_path, "MediaFoundation initialise");
    }

    WCHAR wpath[MAX_PATH+1];
    MultiByteToWideChar(CP_ACP, 0, path.toLocal8Bit().data(), -1, wpath, MAX_PATH);

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
    SafeRelease(&partialType);

    if (FAILED(m_d->err)) {
        m_error = "MediaFoundationReadStream: File format could not be converted to PCM stream";
        throw FileOperationFailed(m_path, "MediaFoundation media type selection");
    }
}

size_t
MediaFoundationReadStream::getFrames(size_t count, float *frames)
{
    cerr << "MediaFoundationReadStream::getFrames(" << count << ")" << endl;
    m_d->mutex.lock();

    while (!m_d->buffer ||
           m_d->buffer->getReadSpace() < count * m_channelCount) {

        m_d->mutex.unlock();

        if (m_d->complete) {
            cerr << "(complete)" << endl;
            if (!m_d->buffer) return 0;
            else return m_d->buffer->read(frames, count * m_channelCount)
                     / m_channelCount;
        }   

        m_d->dataAvailable.lock();
        if (!m_d->complete &&
            (!m_d->buffer ||
             m_d->buffer->getReadSpace() < count * m_channelCount)) {
            m_d->dataAvailable.wait(500000);
        }
        m_d->dataAvailable.unlock();
            
        m_d->mutex.lock();
    }
    
    size_t sz = m_d->buffer->read
        (frames, count * m_channelCount) / m_channelCount;

    m_d->mutex.unlock();
    return sz;
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

HRESULT APIENTRY
MediaFoundationReadStream::D::BufferCB(double sampleTime, BYTE *, long)
{
    //!!! This is still the DirectShow code. We need to replace it
    //!!! with a read thread that fills the ring buffer, waiting each
    //!!! time it becomes full

    std::cerr << "BufferCB: sampleTime = " << sampleTime << std::endl;

    long bufsiz = 0;
    err = grabber->GetCurrentBuffer(&bufsiz, NULL);
    if (FAILED(err)) {
        cerr << "MediaFoundationReadStream: Failed to query sample grabber buffer size" << endl;
        return err;
    }

    cerr << "bufsiz = " << bufsiz << endl;

    if (!rdbuf || rdbufsiz < bufsiz) {
        rdbuf = reallocate<char>(rdbuf, rdbufsiz, bufsiz);
        rdbufsiz = bufsiz;
    }

    char *rdbuf = new char[bufsiz];
    err = grabber->GetCurrentBuffer(&bufsiz, (long *)rdbuf);
    if (FAILED(err)) {
        cerr << "MediaFoundationReadStream: Failed to query sample grabber buffer" << endl;
        return err; 
    }

    // We don't need to know channel count here, just total number of
    // interleaved samples

    int samples = (bufsiz / (bitDepth / 8));

    cerr << "Have " << samples << " samples (across " << channelCount << " channels)" << endl;

    if (!isFloat) {
        if (!cvbuf || cvbufsiz < samples) {
            cvbuf = reallocate<float>(cvbuf, cvbufsiz, samples);
            cvbufsiz = samples;
        }
    }

    mutex.lock();
    if (!buffer) buffer = new RingBuffer<float>(samples);
    else if (buffer->getWriteSpace() < samples) {
        int resizeTo = buffer->getReadSpace() + samples;
        if (resizeTo > maxBufferSize) {
            resizeTo = maxBufferSize;
        }
        RingBuffer<float> *newbuf = buffer->resized(resizeTo);
        cerr << "growing ring buffer to " << buffer->getReadSpace() + samples << endl;
        delete buffer;
        buffer = newbuf;
        while (buffer->getWriteSpace() < samples) {
            mutex.unlock();
            spaceAvailable.lock();
            if (buffer->getWriteSpace() < samples) {
                spaceAvailable.wait(500000);
            }
            spaceAvailable.unlock();
            mutex.lock();
        }
    }
    mutex.unlock();

    if (isFloat) {
        buffer->write((float *)rdbuf, samples);
    } else {
        unsigned char *c = (unsigned char *)rdbuf;
        for (int i = 0; i < samples; ++i) {
            cvbuf[i] = convertSample(c);
            c += (bitDepth / 8);
        }
        buffer->write(cvbuf, samples);
    }

    mutex.lock();
    long ec = 0;
    LONG_PTR p1, p2;
    while ((err = mediaEvent->GetEvent(&ec, &p1, &p2, 0)) == S_OK) {
        if (ec == EC_COMPLETE) {
            cerr << "Setting complete to true" << endl;
            complete = true;
        }
    }
    mutex.unlock();

    dataAvailable.lock();
    dataAvailable.signal();
    dataAvailable.unlock();
    
    return S_OK;
}

MediaFoundationReadStream::~MediaFoundationReadStream()
{
    cerr << "MediaFoundationReadStream::~MediaFoundationReadStream" << endl;

    if (m_d->reader) SafeRelease(&m_d->reader);
    if (m_d->mediaType) SafeRelease(&m_d->mediaType);

    delete m_d;
}

}

#endif

