/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "CoreAudioWriteStream.h"

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

#include <iostream>

#include <QDir>

using std::cerr;
using std::endl;

namespace Turbot
{

static 
AudioWriteStreamBuilder<CoreAudioWriteStream>
coreaudiowritebuilder(
    QString("http://breakfastquay.com/rdf/turbot/fileio/CoreAudioWriteStream"),
    QStringList() << "m4a"
    );

class CoreAudioWriteStream::D
{
public:
    D() : file(0) { }

    ExtAudioFileRef              file;
    AudioBufferList              buffer;
    OSStatus                     err; 
    AudioStreamBasicDescription  asbd;
};

static QString
codestr(OSStatus err)
{
    char text[5];
    UInt32 uerr = err;
    text[0] = (uerr >> 24) & 0xff;
    text[1] = (uerr >> 16) & 0xff;
    text[2] = (uerr >> 8) & 0xff;
    text[3] = (uerr) & 0xff;
    text[4] = '\0';
    return QString("%1 (%2)").arg(err).arg(QString::fromAscii(text));
}

CoreAudioWriteStream::CoreAudioWriteStream(Target target) :
    AudioWriteStream(target),
    m_d(new D)
{
    cerr << "CoreAudioWriteStream: file is " << getPath() << ", channel count is " << getChannelCount() << ", sample rate " << getSampleRate() << endl;

    UInt32 propsize = sizeof(AudioStreamBasicDescription);

    m_d->asbd.mFormatID = kAudioFormatMPEG4AAC;
    m_d->asbd.mSampleRate = getSampleRate();
    m_d->asbd.mChannelsPerFrame = getChannelCount();
    m_d->err = AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, 0,
                                      &propsize, &m_d->asbd);

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
        cerr << m_error << endl;
        throw FailedToWriteFile(getPath());
    }

    QByteArray fba = QFileInfo(getPath()).fileName().toUtf8();
    CFStringRef filename = CFStringCreateWithBytes
	(kCFAllocatorDefault,
	 (const UInt8 *)fba.data(),
         (CFIndex)fba.length(),
	 kCFStringEncodingUTF8,
	 false);

    // Unlike ExtAudioFileCreateWithURL, ExtAudioFileCreateNew
    // apparently cannot be told to overwrite an existing file
    QFile qfi(getPath());
    if (qfi.exists()) qfi.remove();

    m_d->err = ExtAudioFileCreateNew
	(&dref,
	 filename,
	 kAudioFileM4AType,
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

    UInt32 flags = kAudioFileFlags_EraseFile;

    m_d->err = ExtAudioFileCreateWithURL
	(url,
	 kAudioFileM4AType,
	 &m_d->asbd,
	 0,
	 flags,
	 &m_d->file);
    
    CFRelease(url);
#endif

    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Failed to create file: code " + codestr(m_d->err);
        cerr << m_error << endl;
        throw FailedToWriteFile(getPath());
    }

    memset(&m_d->asbd, 0, sizeof(AudioStreamBasicDescription));
    propsize = sizeof(AudioStreamBasicDescription);
    m_d->asbd.mSampleRate = getSampleRate();
    m_d->asbd.mFormatID = kAudioFormatLinearPCM;
    m_d->asbd.mChannelsPerFrame = getChannelCount();
    AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, 0,
                           &propsize, &m_d->asbd);
    m_d->asbd.mFormatFlags =
        kAudioFormatFlagIsFloat |
        kAudioFormatFlagIsPacked |
        kAudioFormatFlagsNativeEndian;
    m_d->asbd.mFramesPerPacket = 1;
    m_d->asbd.mBitsPerChannel = sizeof(float) * 8;
    m_d->asbd.mBytesPerFrame = sizeof(float) * getChannelCount();
    m_d->asbd.mBytesPerPacket = sizeof(float) * getChannelCount();
	
    m_d->err = ExtAudioFileSetProperty
	(m_d->file, kExtAudioFileProperty_ClientDataFormat,
         sizeof(AudioStreamBasicDescription), &m_d->asbd);
    
    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Error in setting client format: code " + codestr(m_d->err);
        cerr << m_error << endl;
        throw FileOperationFailed(getPath(), "set client format");
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

void
CoreAudioWriteStream::putInterleavedFrames(size_t count, float *frames)
{
    if (count == 0) return;

    m_d->buffer.mBuffers[0].mDataByteSize =
        sizeof(float) * getChannelCount() * count;
    
    m_d->buffer.mBuffers[0].mData = frames;

    UInt32 framesToWrite = count;

    m_d->err = ExtAudioFileWrite(m_d->file, framesToWrite, &m_d->buffer);
    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Error in encoder: code " + codestr(m_d->err);
        cerr << m_error << endl;
        throw FileOperationFailed(getPath(), "encode");
    }
}

}

#endif
