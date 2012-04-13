/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "CoreAudioWriteStream.h"

#ifdef HAVE_COREAUDIO

#include <iostream>

using std::cerr;
using std::endl;

namespace Turbot
{

static 
AudioWriteStreamBuilder<CoreAudioWriteStream>
simplewavbuilder(
    QString("http://breakfastquay.com/rdf/turbot/fileio/CoreAudioWriteStream"),
    QStringList() << "mp3"
    );

class CoreAudioWriteStream::D
{
public:
    D() : file(0) { }

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

CoreAudioWriteStream::CoreAudioWriteStream(Target target) :
    AudioWriteStream(target),
    m_d(new D)
{
    m_d->asbd.mSampleRate = getSampleRate();
    m_d->asbd.mFormatID = kAudioFormatMPEGLayer3;
    m_d->asbd.mFormatFlags = 0;
    m_d->asbd.mBytesPerPacket = 0; //???
    m_d->asbd.mFramesPerPacket = 0; //???
    m_d->asbd.mBytesPerFrame = 0;
    m_d->asbd.mChannelsPerFrame = getChannelCount();
    m_d->asbd.mBitsPerChannel = 0;
    m_d->asbd.mReserved = 0;

    UInt32 flags = kAudioFileFlags_EraseFile;

#if (MAC_OS_X_VERSION_MIN_REQUIRED <= 1040)

    QDir dir = QFileInfo(getPath()).dir();
    QByteArray dba = dir.absolutePath().toLocal8Bit();

    CFURLRef durl = CFURLCreateFromFileSystemRepresentation
        (kCFAllocatorDefault,
         (const UInt8 *)dba.data(),
         (CFIndex)dba.length(),
         false);

    FSRef dref;
    if (!CFURLGetFSRef(durl, &dref)) { // returns Boolean, not error code
        m_error = "CoreAudioReadStream: Error looking up FS ref (directory not found?)";
        return;
    }

    QByteArray fba = QFileInfo(getPath()).fileName().toUtf8();
    CFStringRef filename = CFStringCreateWithBytes
	(kCFAllocatorDefault,
	 (const UInt8 *)fba.data(),
         (CFIndex)fba.length(),
	 kCFStringEncodingUTF8,
	 false);
    
    m_d->err = ExtAudioFileCreateNew
	(&dref,
	 filename,
	 kAudioFileMP3Type,
	 &m_d->asbd,
	 0,
	 &m_d->file);

    CFRelease(durl);
    CFRelease(filename);
#else
    QByteArray ba = getPath().toLocal8Bit();

    CFURLRef url = CFURLCreateFromFileSystemRepresentation
        (kCFAllocatorDefault,
         (const UInt8 *)ba.data(),
         (CFIndex)ba.length(),
         false);

    m_d->err = ExtAudioFileCreateWithURL
	(url,
	 kAudioFileMP3Type,
	 &m_d->asbd,
	 0,
	 flags,
	 &m_d->file);
    
    CFRelease(url);
#endif

    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Failed to create file: code " + codestr(m_d->err);
        return;
    }

    m_d->asbd.mSampleRate = getSampleRate();
    m_d->asbd.mFormatID = kAudioFormatLinearPCM;
    m_d->asbd.mFormatFlags =
        kAudioFormatFlagIsFloat |
        kAudioFormatFlagIsPacked |
        kAudioFormatFlagsNativeEndian;
    m_d->asbd.mBytesPerPacket = m_d->asbd.mBytesPerFrame;
    m_d->asbd.mFramesPerPacket = 1;
    m_d->asbd.mBitsPerChannel = sizeof(float) * 8;
    m_d->asbd.mBytesPerFrame = sizeof(float) * getChannelCount();
    m_d->asbd.mChannelsPerFrame = getChannelCount();
    m_d->asbd.mReserved = 0;
	
    UInt32 propsize = sizeof(AudioStreamBasicDescription);
    m_d->err = ExtAudioFileSetProperty
	(m_d->file, kExtAudioFileProperty_ClientDataFormat, propsize, &m_d->asbd);
    
    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Error in setting client format: code " + codestr(m_d->err);
        return;
    }

    m_d->buffer.mNumberBuffers = 1;
    m_d->buffer.mBuffers[0].mNumberChannels = getChannelCount();
    m_d->buffer.mBuffers[0].mDataByteSize = 0;
    m_d->buffer.mBuffers[0].mData = 0;
}

CoreAudioWriteStream::~CoreAudioWriteStream()
{
    if (m_d->file) {
	ExtAudioFileDispose(m_d->file);
    }
}

bool
CoreAudioWriteStream::putInterleavedFrames(size_t count, float *frames)
{
    if (!m_d->file || !getChannelCount()) return false;
    if (count == 0) return true;

    m_d->buffer.mBuffers[0].mDataByteSize =
        sizeof(float) * m_channelCount * count;
    
    m_d->buffer.mBuffers[0].mData = frames;

    UInt32 framesToWrite = count;

    m_d->err = ExtAudioFileWrite(m_d->file, &framesToWrite, &m_d->buffer);
    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Error in encoder: code " + codestr(m_d->err);
	return false;
    }
                
    return true;
}

}

#endif
