/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_QUICKTIME

#include "QuickTimeReadStream.h"

#include <QuickTime/QuickTime.h>

#include <iostream>
#include <string>

namespace Turbot
{

static AudioReadStreamBuilder<QuickTimeReadStream>
builder(QuickTimeReadStream::getUri());

QUrl
QuickTimeReadStream::getUri()
{
    return QUrl("http://breakfastquay.com/rdf/turbot/fileio/QuickTimeReadStream");
}

class QuickTimeReadStream::D
{
public:
    D() : movie(0) { }

    MovieAudioExtractionRef      extractionSessionRef;
    AudioBufferList              buffer;
    OSErr                        err; 
    AudioStreamBasicDescription  asbd;
    Movie                        movie;
};

static std::string
codestr(OSErr err)
{
    static char buffer[20];
    sprintf(buffer, "%ld", (long)err);
    return std::string(buffer);
}

QuickTimeReadStream::QuickTimeReadStream(std::string path) :
    m_path(path),
    m_d(new D)
{
    m_channelCount = 0;
    m_sampleRate = 0;

    long QTversion;

    m_d->err = Gestalt(gestaltQuickTime,&QTversion);
    if ((m_d->err != noErr) || (QTversion < 0x07000000)) {
        m_error = "QuickTimeReadStream: Failed to find a suitable version of QuickTime (version 7 or above required)";
        return;
    }

    EnterMovies();
	
    Handle dataRef; 
    OSType dataRefType;

    CFURLRef url = CFURLCreateFromFileSystemRepresentation
        (kCFAllocatorDefault,
         (const UInt8 *)m_path.c_str(),
         (CFIndex)m_path.length(),
         false);

    m_d->err = QTNewDataReferenceFromCFURL
        (url, 0, &dataRef, &dataRefType);

    if (m_d->err) { 
        m_error = "QuickTimeReadStream: Error creating data reference: code " + codestr(m_d->err);
        return;
    }
    
    short fileID = movieInDataForkResID; 
    short flags = 0; 
    m_d->err = NewMovieFromDataRef
        (&m_d->movie, flags, &fileID, dataRef, dataRefType);

    DisposeHandle(dataRef);
    if (m_d->err) { 
        m_error = "QuickTimeReadStream: Error creating new movie: code " + codestr(m_d->err); 
        return;
    }

    Boolean isProtected = 0;
    Track aTrack = GetMovieIndTrackType
        (m_d->movie, 1, SoundMediaType,
         movieTrackMediaType | movieTrackEnabledOnly);

    if (aTrack) {
        Media aMedia = GetTrackMedia(aTrack);	// get the track media
        if (aMedia) {
            MediaHandler mh = GetMediaHandler(aMedia);	// get the media handler we can query
            if (mh) {
                m_d->err = QTGetComponentProperty(mh,
                                                  kQTPropertyClass_DRM,
                                                  kQTDRMPropertyID_IsProtected,
                                                  sizeof(Boolean), &isProtected,nil);
            } else {
                m_d->err = 1;
            }
        } else {
            m_d->err = 1;
        }
    } else {
        m_d->err = 1;
    }
	
    if (m_d->err && m_d->err != kQTPropertyNotSupportedErr) { 
        m_error = "QuickTimeReadStream: Error checking for DRM in QuickTime decoder: code " + codestr(m_d->err);
        return;
    } else if (!m_d->err && isProtected) { 
        throw FileDRMProtected(path);
    }

    if (m_d->movie) {
        SetMovieActive(m_d->movie, TRUE);
        m_d->err = GetMoviesError();
        if (m_d->err) {
            m_error = "QuickTimeReadStream: Error in decoder activation: code " + codestr(m_d->err);
            return;
        }
    } else {
	m_error = "QuickTimeReadStream: Error in decoder: Movie object not valid";
	return;
    }
    
    m_d->err = MovieAudioExtractionBegin
        (m_d->movie, 0, &m_d->extractionSessionRef);
    if (m_d->err) {
        m_error = "QuickTimeReadStream: Error in decoder extraction init: code " + codestr(m_d->err);
        return;
    }

    m_d->err = MovieAudioExtractionGetProperty
        (m_d->extractionSessionRef,
         kQTPropertyClass_MovieAudioExtraction_Audio, kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
         sizeof(m_d->asbd),
         &m_d->asbd,
         nil);

    if (m_d->err) {
        m_error = "QuickTimeReadStream: Error in decoder property get: code " + codestr(m_d->err);
        return;
    }
	
    m_channelCount = m_d->asbd.mChannelsPerFrame;
    m_sampleRate = m_d->asbd.mSampleRate;

//    std::cerr << "QuickTimeReadStream: " << m_channelCount << " channels, " << m_sampleRate << " Hz" << std::endl;

    m_d->asbd.mFormatFlags =
        kAudioFormatFlagIsFloat |
        kAudioFormatFlagIsPacked |
        kAudioFormatFlagsNativeEndian;
    m_d->asbd.mBitsPerChannel = sizeof(float) * 8;
    m_d->asbd.mBytesPerFrame = sizeof(float) * m_d->asbd.mChannelsPerFrame;
    m_d->asbd.mBytesPerPacket = m_d->asbd.mBytesPerFrame;
	
    m_d->err = MovieAudioExtractionSetProperty
        (m_d->extractionSessionRef,
         kQTPropertyClass_MovieAudioExtraction_Audio,
         kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
         sizeof(m_d->asbd),
         &m_d->asbd);

    if (m_d->err) {
        m_error = "QuickTimeReadStream: Error in decoder property set: code " + codestr(m_d->err);
        m_channelCount = 0; // so isOK() returns false
        return;
    }
    m_d->buffer.mNumberBuffers = 1;
    m_d->buffer.mBuffers[0].mNumberChannels = m_channelCount;
    m_d->buffer.mBuffers[0].mDataByteSize = 0;
    m_d->buffer.mBuffers[0].mData = 0;
}

size_t
QuickTimeReadStream::getInterleavedFrames(size_t count, float *frames)
{
    if (!m_channelCount) return 0;
    if (count == 0) return 0;

    m_d->buffer.mBuffers[0].mDataByteSize =
        sizeof(float) * m_channelCount * count;
    
    m_d->buffer.mBuffers[0].mData = frames;

    UInt32 framesRead = count;
    UInt32 extractionFlags = 0;

    m_d->err = MovieAudioExtractionFillBuffer
        (m_d->extractionSessionRef, &framesRead, &m_d->buffer, &extractionFlags);
    if (m_d->err) {
        m_error = "QuickTimeReadStream: Error in decoder: code " + codestr(m_d->err);
    }

    return framesRead;
}

QuickTimeReadStream::~QuickTimeReadStream()
{
//    std::cerr << "QuickTimeReadStream::~QuickTimeReadStream" << std::endl;

    if (m_d->movie) {
        SetMovieActive(m_d->movie, FALSE);
        DisposeMovie(m_d->movie);
    }

    delete m_d;
}

std::vector<std::string>
QuickTimeReadStream::getSupportedFileExtensions()
{
    const char *exts[] = {
	"aiff", "aif", "au", "avi", "m4a", "m4b", "m4p", "m4v",
	"mov", "mp3", "mp4", "wav"
    };
    std::vector<std::string> rv;
    for (int i = 0; i < sizeof(exts)/sizeof(exts[0]); ++i) {
	rv.push_back(exts[i]);
    }
    return rv;
}

}

#endif

