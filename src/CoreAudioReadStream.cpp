/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_COREAUDIO

#include "CoreAudioReadStream.h"

#include "system/Debug.h"

#if !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
#include <AudioToolbox/AudioToolbox.h>
#else
#include "AudioToolbox.h"
#include "ExtendedAudioFile.h"
#endif

#include "CAStreamBasicDescription.h"
#include "CAXException.h"

namespace Turbot
{

static
AudioReadStreamBuilder<CoreAudioReadStream>
coreaudiobuilder(
    QString("http://breakfastquay.com/rdf/turbot/fileio/CoreAudioReadStream"),
    QStringList() << "aiff" << "aif" << "au" << "avi" << "m4a" << "m4b" << "m4p" << "m4v" << "mov" << "mp3" << "mp4" << "wav"
    );

class CoreAudioReadStream::D
{
public:
    D() : data(0) { }

    ExtAudioFileRef              file;
    AudioBufferList              buffer;
    OSErr                        err; 
    AudioStreamBasicDescription  asbd;
};

static QString
codestr(OSErr err)
{
    static char buffer[20];
    sprintf(buffer, "%ld", (long)err);
    return QString(buffer);
}

CoreAudioReadStream::CoreAudioReadStream(QString path) :
    m_path(path),
    m_d(new D)
{
    m_channelCount = 0;
    m_sampleRate = 0;

    QByteArray ba = m_path.toLocal8Bit();

    CFURLRef url = CFURLCreateFromFileSystemRepresentation
        (kCFAllocatorDefault,
         (const UInt8 *)ba.data(),
         (CFIndex)ba.length(),
         false);

    m_d->err = ExtAudioFileOpenURL
	(url, &m_d->file);

    if (m_d->err) { 
        m_error = "CoreAudioReadStream: Error opening file: code " + codestr(m_d->err);
        return;
    }
    
    UInt32 propsize = sizeof(AudioStreamBasicDescription);
    m_d->err = ExtAudioFileGetProperty
	(m_d->file, kAudioFilePropertyDataFormat, &propsize, &m_d->asbd);
    
    if (m_d->err) {
        m_error = "CoreAudioReadStream: Error in getting basic description: code " + codestr(m_d->err);
        return;
    }
	
    m_channelCount = m_d->asbd.mChannelsPerFrame;
    m_sampleRate = m_d->asbd.mSampleRate;

    DEBUG << "CoreAudioReadStream: " << m_channelCount << " channels, " << m_sampleRate << " Hz" << endl;

    m_d->asbd.mFormatID = kAudioFormatLinearPCM;
    m_d->asbd.mFormatFlags =
        kAudioFormatFlagIsFloat |
        kAudioFormatFlagIsPacked |
        kAudioFormatFlagsNativeEndian;
    m_d->asbd.mBitsPerChannel = sizeof(float) * 8;
    m_d->asbd.mBytesPerFrame = sizeof(float) * m_d->asbd.mChannelsPerFrame;
    m_d->asbd.mBytesPerPacket = m_d->asbd.mBytesPerFrame;
	
    m_d->err = ExtAudioFileSetProperty
	(m_d->file, kExtAudioFileProperty_ClientDataFormat, propsize, &m_d->asbd);
    
    if (m_d->err) {
        m_error = "CoreAudioReadStream: Error in setting client format: code " + codestr(m_d->err);
        return;
    }

    m_d->buffer.mNumberBuffers = 1;
    m_d->buffer.mBuffers[0].mNumberChannels = m_channelCount;
    m_d->buffer.mBuffers[0].mDataByteSize = 0;
    m_d->buffer.mBuffers[0].mData = 0;
}

size_t
CoreAudioReadStream::getFrames(size_t count, float *frames)
{
    if (!m_channelCount) return 0;
    if (count == 0) return 0;

    m_d->buffer.mBuffers[0].mDataByteSize =
        sizeof(float) * m_channelCount * count;
    
    m_d->buffer.mBuffers[0].mData = frames;

    UInt32 framesRead = count;
    UInt32 extractionFlags = 0;

    m_d->err = ExtAudioFileRead(m_d->file, &count, &m_d->buffer);
    if (m_d->err) {
        m_error = "CoreAudioReadStream: Error in decoder: code " + codestr(m_d->err);
    }

    return framesRead;
}

CoreAudioReadStream::~CoreAudioReadStream()
{
//    std::cerr << "CoreAudioReadStream::~CoreAudioReadStream" << std::endl;

    if (m_channelCount) {
	ExtAudioFileDispose(m_d->file);
    }

    m_channelCount = 0;

    delete m_d;
}

}

#endif

