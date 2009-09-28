/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_DIRECTSHOW

#include "DirectShowReadStream.h"
#include "base/RingBuffer.h"
#include "system/Thread.h"

#include <windows.h>
#include <objbase.h>
#include <dshow.h>
#include <qedit.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

#include <iostream>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

namespace Turbot
{

static AudioFileReadStreamBuilder<DirectShowReadStream>
builder(DirectShowReadStream::getUri());

QUrl
DirectShowReadStream::getUri()
{
    return QUrl("http://breakfastquay.com/rdf/turbot/fileio/DirectShowReadStream");
}

static const int maxBufferSize = 1048575;

class DirectShowReadStream::D : public ISampleGrabberCB
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
        graphBuilder(0), 
        grabber(0), 
        mediaControl(0), 
        mediaEvent(0),
        complete(false),
        buffer(0),
        dataAvailable("DirectShowReadStream::dataAvailable"),
        spaceAvailable("DirectShowReadStream::spaceAvailable")
        { }

    ~D() {
        if (mediaControl) {
            mediaControl->Stop();
            mediaControl->Release();
        }
        if (mediaEvent) {
            mediaEvent->Release();
        }
        if (grabber) {
            grabber->Release();
        }
        if (graphBuilder) {
            graphBuilder->Release();
        }
        deallocate<char>(rdbuf);
        deallocate<float>(cvbuf);
        delete buffer;
    }

	HRESULT APIENTRY QueryInterface(REFIID iid, void **obj) {
		if (iid == IID_IUnknown) {
			*obj = dynamic_cast<IUnknown*>(this);
		} else if (iid == IID_ISampleGrabberCB) {
			*obj = dynamic_cast<ISampleGrabberCB*>(this);
		} else {
			*obj = NULL;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}

	ULONG APIENTRY AddRef() {
		return ++refCount;
	}

	ULONG APIENTRY Release() {
		return --refCount;
	}

    HRESULT APIENTRY SampleCB(double, IMediaSample *) {
        return S_OK;
    }
    HRESULT APIENTRY BufferCB(double, BYTE *, long);

    ULONG refCount;

    size_t channelCount;
    size_t bitDepth;
    size_t sampleRate;
    bool isFloat;

    char *rdbuf;
    int rdbufsiz;
    
    float *cvbuf;
    int cvbufsiz;

    HRESULT err;
    IGraphBuilder *graphBuilder;
    ISampleGrabber *grabber;
    IMediaControl *mediaControl;
    IMediaEvent *mediaEvent;

    bool complete;

    RingBuffer<float> *buffer;
    Mutex mutex;
    Condition dataAvailable;
    Condition spaceAvailable;

    float convertSample(const unsigned char *);
    void fillBuffer();
};

// The following functions (GetUnconnectedPin and the two
// ConnectFilters implementations) are taken verbatim from MSDN
// documentation for DirectShow.  The MSDN terms of use says:
// "If Microsoft makes Software available on this Web Site without
// a License Agreement, you may use such Software to design,
// develop and test your programs to run on Microsoft products
// and services."  I hope that covers it.

static HRESULT GetUnconnectedPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    PIN_DIRECTION PinDir,   // Direction of the pin to find.
    IPin **ppPin)           // Receives a pointer to the pin.
{
    *ppPin = 0;
    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }
    while (pEnum->Next(1, &pPin, NULL) == S_OK)
    {
        PIN_DIRECTION ThisPinDir;
        pPin->QueryDirection(&ThisPinDir);
        if (ThisPinDir == PinDir)
        {
            IPin *pTmp = 0;
            hr = pPin->ConnectedTo(&pTmp);
            if (SUCCEEDED(hr))  // Already connected, not the pin we want.
            {
                pTmp->Release();
            }
            else  // Unconnected, this is the pin we want.
            {
                pEnum->Release();
                *ppPin = pPin;
                return S_OK;
            }
        }
        pPin->Release();
    }
    pEnum->Release();
    // Did not find a matching pin.
    return E_FAIL;
}

static HRESULT ConnectFilters(
    IGraphBuilder *pGraph, // Filter Graph Manager.
    IPin *pOut,            // Output pin on the upstream filter.
    IBaseFilter *pDest)    // Downstream filter.
{
    if ((pGraph == NULL) || (pOut == NULL) || (pDest == NULL))
    {
        return E_POINTER;
    }
#ifdef DEBUG
        PIN_DIRECTION PinDir;
        pOut->QueryDirection(&PinDir);
        _ASSERTE(PinDir == PINDIR_OUTPUT);
#endif

    // Find an input pin on the downstream filter.
    IPin *pIn = 0;
    HRESULT hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
    if (FAILED(hr))
    {
        return hr;
    }
    // Try to connect them.
    hr = pGraph->Connect(pOut, pIn);
    pIn->Release();
    return hr;
}

static HRESULT ConnectFilters(
    IGraphBuilder *pGraph, 
    IBaseFilter *pSrc, 
    IBaseFilter *pDest)
{
    if ((pGraph == NULL) || (pSrc == NULL) || (pDest == NULL))
    {
        return E_POINTER;
    }

    // Find an output pin on the first filter.
    IPin *pOut = 0;
    HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (FAILED(hr)) 
    {
        return hr;
    }
    hr = ConnectFilters(pGraph, pOut, pDest);
    pOut->Release();
    return hr;
}

DirectShowReadStream::DirectShowReadStream(string path) :
    m_path(path),
    m_d(new D)
{
    m_channelCount = 0;
    m_sampleRate = 0;

    cerr << "DirectShowReadStream(" << path << ")" << endl;

    // c.f. http://msdn.microsoft.com/en-gb/library/ms787867(VS.85).aspx

    // Note: CoInitializeEx(NULL, COINIT_MULTITHREADED) must
    // presumably already have been called

    m_d->err = CoCreateInstance(CLSID_FilterGraph,
                                NULL,
                                CLSCTX_ALL,
                                IID_IGraphBuilder,
                                (void **)&(m_d->graphBuilder));
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to initialise DirectShow graph builder";
        return;
    }

    IBaseFilter *grabberFilter = 0;

    m_d->err = CoCreateInstance(CLSID_SampleGrabber,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IBaseFilter, (void**)&grabberFilter);  
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to initialise sample grabber";
        return;
    }

    m_d->err = m_d->graphBuilder->AddFilter(grabberFilter, L"Sample Grabber");
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to add sample grabber to filter graph";
        return;
    }

    IBaseFilter *nullFilter = 0;

    m_d->err = CoCreateInstance(CLSID_NullRenderer,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IBaseFilter, (void**)&nullFilter);  

    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to initialise null renderer";
        return;
    }

    m_d->err = m_d->graphBuilder->AddFilter(nullFilter, L"Null Renderer");
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to add null renderer to filter graph";
        return;
    }

    grabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&(m_d->grabber));

    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Audio;

    // We'd ideally like IEEE_FLOAT, but if we ask for that here, the
    // graph fails to connect any source that requires decoding (it
    // doesn't seem to be able to set up e.g. mp3 -> decoder ->
    // pcm-float converter -> sample grabber).  Whereas if we specify
    // PCM with no particular bit depth, it connects fine.
    mt.subtype = MEDIASUBTYPE_PCM;

    m_d->err = m_d->grabber->SetMediaType(&mt);
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to set sample grabber media type";
        return;
    }

    WCHAR wpath[MAX_PATH+1];
    MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, wpath, MAX_PATH);

    IBaseFilter *src;
    m_d->err = m_d->graphBuilder->AddSourceFilter(wpath, L"Source", &src);
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to open source file";
        return;
    }

    m_d->err = ConnectFilters(m_d->graphBuilder, src, grabberFilter);
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to connect source filter";
        return;
    }

    m_d->err = ConnectFilters(m_d->graphBuilder, grabberFilter, nullFilter);
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to connect null renderer";
        return;
    }

    IMediaFilter *mediaFilter = 0;
    m_d->err = m_d->graphBuilder->QueryInterface
        (IID_IMediaFilter, (void**)&mediaFilter);
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to query media filter interface";
        return;
    }

    mediaFilter->SetSyncSource(NULL); // no clock: run as fast as you can

    m_d->err = m_d->grabber->SetBufferSamples(TRUE);
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to set grabber buffer mode";
        return;
    }

//    m_d->err = m_d->grabber->SetOneShot(TRUE); //???
  //  if (FAILED(m_d->err)) {
    //    m_error = "DirectShowReadStream: Failed to set grabber read mode";
      //  return;
    //}

    static GUID guids[] = {
        // This is only here for debug purposes, so we can actually
        // look at the GUID values in a debugger
        MEDIASUBTYPE_PCM,
        MEDIASUBTYPE_MPEG1Packet,
        MEDIASUBTYPE_MPEG1Payload,
        MEDIASUBTYPE_MPEG2_AUDIO,
        MEDIASUBTYPE_DVD_LPCM_AUDIO,
        MEDIASUBTYPE_MPEG2_AUDIO,
        MEDIASUBTYPE_DRM_Audio,
        MEDIASUBTYPE_IEEE_FLOAT,
        MEDIASUBTYPE_DOLBY_AC3,
        MEDIASUBTYPE_DOLBY_AC3_SPDIF,
        MEDIASUBTYPE_RAW_SPORT,
        MEDIASUBTYPE_SPDIF_TAG_241h,
        MEDIASUBTYPE_MPEG1Audio,
        MEDIASUBTYPE_None
    };

    m_d->err = m_d->grabber->GetConnectedMediaType(&mt);
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to query connected media type";
        return;
    }
    if (mt.majortype != MEDIATYPE_Audio) {
        m_error = "DirectShowReadStream: Connected medium is not audio type";
        return;
    }
    if (mt.subtype == MEDIASUBTYPE_DRM_Audio) {
        throw FileDRMProtected(path);
    }
    if (mt.subtype == MEDIASUBTYPE_PCM) {
        cerr << "Note: DirectShowReadStream: Connected medium has subtype PCM" << endl;
    } else if (mt.subtype == MEDIASUBTYPE_IEEE_FLOAT) {
        cerr << "Note: DirectShowReadStream: Connected medium has subtype IEEE Float" << endl;
    } else {
        m_error = "DirectShowReadStream: Connected medium has unsupported subtype";
        return;
    }

    cerr << "Note: DirectShowReadStream: Connected medium has sample size "
         << mt.lSampleSize << endl;
    int sampleSize = mt.lSampleSize;

    if (mt.formattype == FORMAT_WaveFormatEx) {
        cerr << "Note: DirectShowReadStream: Connected medium has WAVE format type" << endl;

        WAVEFORMATEX *w = (WAVEFORMATEX *)mt.pbFormat;
        m_channelCount = w->nChannels;
        m_sampleRate = w->nSamplesPerSec;

        m_d->channelCount = m_channelCount;
        m_d->sampleRate = m_sampleRate;

        m_d->bitDepth = w->wBitsPerSample;
        m_d->isFloat = (w->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);

        if (w->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            WAVEFORMATEXTENSIBLE *ew = (WAVEFORMATEXTENSIBLE *)w;
            m_d->isFloat = (ew->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
        }

        cerr << "Note: Queried WAVE data: " << m_channelCount << " channels, " << m_sampleRate << "kHz, " << m_d->bitDepth << " bits per sample, float = " << m_d->isFloat << endl;

        if (m_d->bitDepth % 8 != 0) {
            m_error = "DirectShowReadStream: Connected medium has non-integer bytes per sample, cannot handle this";
            return;
        }
        if (m_d->bitDepth != 8 && m_d->bitDepth != 16 && m_d->bitDepth != 24) {
            if (!m_d->isFloat) {
                m_error = "DirectShowReadStream: Connected medium has PCM data at unsupported bit depth (only 8, 16, 24 bits per sample are supported for non-float data types)";
                return;
            }
        }

    } else {
        m_error = "DirectShowReadStream: Connected medium has unsupported format type";
        return;
    }
    
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }

    m_d->grabber->SetCallback(m_d, 1);

    m_d->err = m_d->graphBuilder->QueryInterface
        (IID_IMediaControl, (void**)&(m_d->mediaControl));
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to query media control interface";
        return;
    }

    m_d->err = m_d->graphBuilder->QueryInterface
        (IID_IMediaEvent, (void**)&(m_d->mediaEvent));
    if (FAILED(m_d->err)) {
        m_error = "DirectShowReadStream: Failed to query media event interface";
        return;
    }

    m_d->mediaControl->Run();
}

size_t
DirectShowReadStream::getInterleavedFrames(size_t count, float *frames)
{
    cerr << "DirectShowReadStream::getInterleavedFrames(" << count << ")" << endl;
    m_d->mutex.lock();

    while (!m_d->buffer ||
           m_d->buffer->getReadSpace() < count * m_channelCount) {

        long ec = 0;
        LONG_PTR p1, p2;
        HRESULT err;
        while ((err = m_d->mediaEvent->GetEvent(&ec, &p1, &p2, 0)) == S_OK) {
            if (ec == EC_COMPLETE) {
                cerr << "Setting complete to true" << endl;
                m_d->complete = true;
            }
        }

        m_d->mutex.unlock();
        if (m_d->complete) {
            cerr << "(complete)" << endl;
            if (!m_d->buffer) return 0;
            else return m_d->buffer->read(frames, count * m_channelCount)
                     / m_channelCount;
        }   
        cerr << "(not enough data yet)" << endl;

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
DirectShowReadStream::D::convertSample(const unsigned char *c)
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
DirectShowReadStream::D::BufferCB(double sampleTime, BYTE *, long)
{
    std::cerr << "BufferCB: sampleTime = " << sampleTime << std::endl;

    long bufsiz = 0;
    err = grabber->GetCurrentBuffer(&bufsiz, NULL);
    if (FAILED(err)) {
        cerr << "DirectShowReadStream: Failed to query sample grabber buffer size" << endl;
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
        cerr << "DirectShowReadStream: Failed to query sample grabber buffer" << endl;
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
        //cerr << "values: ";
        for (int i = 0; i < samples; ++i) {
            cvbuf[i] = convertSample(c);
          //  cerr << cvbuf[i] << " ";
           // if (i % 8 == 0) cerr << endl;
            c += (bitDepth / 8);
        }
     //   cerr << endl;
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

DirectShowReadStream::~DirectShowReadStream()
{
    cerr << "DirectShowReadStream::~DirectShowReadStream" << endl;

    //!!! clean up properly!

    delete m_d;
}

vector<string>
DirectShowReadStream::getSupportedFileExtensions()
{
    vector<string> extensions;
    int count;
    
    //!!!
    extensions.push_back("wav");
    extensions.push_back("mp3");
    extensions.push_back("wma");
    extensions.push_back("ogg");
    return extensions;
}

}

#endif

