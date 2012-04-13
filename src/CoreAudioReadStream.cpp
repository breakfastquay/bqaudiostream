/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_COREAUDIO

// OS/X system headers don't cope with DEBUG
#ifdef DEBUG
#undef DEBUG
#endif

#if !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
#include <AudioToolbox/AudioToolbox.h>
#include <AudioToolbox/ExtendedAudioFile.h>
#else
#include "AudioToolbox.h"
#include "ExtendedAudioFile.h"
#endif

#include "CoreAudioReadStream.h"

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
    D() { }

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

#if (MAC_OS_X_VERSION_MIN_REQUIRED <= 1040)
    FSRef fsref;
    if (!CFURLGetFSRef(url, &fsref)) { // returns Boolean, not error code
        m_error = "CoreAudioReadStream: Error looking up FS ref (file not found?)";
        return;
    }
    m_d->err = ExtAudioFileOpen(&fsref, &m_d->file);
#else
    m_d->err = ExtAudioFileOpenURL(url, &m_d->file);
#endif

    CFRelease(url);

    if (m_d->err) { 
        m_error = "CoreAudioReadStream: Error opening file: code " + codestr(m_d->err);
        return;
    }
    
    UInt32 propsize = sizeof(AudioStreamBasicDescription);
    m_d->err = ExtAudioFileGetProperty
	(m_d->file, kExtAudioFileProperty_FileDataFormat, &propsize, &m_d->asbd);
    
    if (m_d->err) {
        m_error = "CoreAudioReadStream: Error in getting basic description: code " + codestr(m_d->err);
        return;
    }
	
    m_channelCount = m_d->asbd.mChannelsPerFrame;
    m_sampleRate = m_d->asbd.mSampleRate;

    std::cerr << "CoreAudioReadStream: " << m_channelCount << " channels, " << m_sampleRate << " Hz" << std::endl;

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

    m_d->err = ExtAudioFileRead(m_d->file, &framesRead, &m_d->buffer);
    if (m_d->err) {
        m_error = "CoreAudioReadStream: Error in decoder: code " + codestr(m_d->err);
    }

 //   std::cerr << "CoreAudioReadStream::getFrames: " << count << " frames requested across " << m_channelCount << " channel(s), " << framesRead << " frames actually read" << std::endl;

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

