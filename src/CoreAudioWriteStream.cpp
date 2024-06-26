/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/*
    bqaudiostream

    A small library wrapping various audio file read/write
    implementations in C++.

    Copyright 2007-2022 Particular Programs Ltd.

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the names of Chris Cannam and
    Particular Programs Ltd shall not be used in advertising or
    otherwise to promote the sale, use or other dealings in this
    Software without prior written authorization.
*/

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
#include <sstream>

namespace breakfastquay
{

static std::vector<std::string>
getCoreAudioWriteExtensions()
{
    std::vector<std::string> extensions;
    extensions.push_back("m4a");
    return extensions;
}

static 
AudioWriteStreamBuilder<CoreAudioWriteStream>
coreaudiowritebuilder(
    std::string("http://breakfastquay.com/rdf/turbot/audiostream/CoreAudioWriteStream"),
    getCoreAudioWriteExtensions()
    );

class CoreAudioWriteStream::D
{
public:
    D() : file(0) { }

    AudioStreamBasicDescription  asbd;
    ExtAudioFileRef              file;
    AudioBufferList              buffer;
    OSStatus                     err; 
};

static std::string
codestr(OSStatus err)
{
    char text[5];
    UInt32 uerr = err;
    text[0] = (uerr >> 24) & 0xff;
    text[1] = (uerr >> 16) & 0xff;
    text[2] = (uerr >> 8) & 0xff;
    text[3] = (uerr) & 0xff;
    text[4] = '\0';
    std::ostringstream os;
    os << err << " (" << text << ")";
    return os.str();
}

CoreAudioWriteStream::CoreAudioWriteStream(Target target) :
    AudioWriteStream(target),
    m_d(new D)
{
//    cerr << "CoreAudioWriteStream: file is " << getPath() << ", channel count is " << getChannelCount() << ", sample rate " << getSampleRate() << endl;

    UInt32 propsize = sizeof(AudioStreamBasicDescription);

    memset(&m_d->asbd, 0, sizeof(AudioStreamBasicDescription));
    m_d->asbd.mFormatID = kAudioFormatMPEG4AAC;
    m_d->asbd.mSampleRate = getSampleRate();
    m_d->asbd.mChannelsPerFrame = getChannelCount();
    m_d->err = AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, 0,
                                      &propsize, &m_d->asbd);


    std::string path = getPath();
    CFURLRef url = CFURLCreateFromFileSystemRepresentation
        (kCFAllocatorDefault,
         (const UInt8 *)path.c_str(),
         (CFIndex)path.size(),
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

    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Failed to create file: code " + codestr(m_d->err);
//        cerr << m_error << endl;
        throw FailedToWriteFile(getPath());
    }

    memset(&m_d->asbd, 0, sizeof(AudioStreamBasicDescription));
    propsize = sizeof(AudioStreamBasicDescription);
    m_d->asbd.mSampleRate = getSampleRate();
    m_d->asbd.mFormatID = kAudioFormatLinearPCM;
    m_d->asbd.mChannelsPerFrame = getChannelCount();
    m_d->asbd.mFormatFlags =
        kAudioFormatFlagIsFloat |
        kAudioFormatFlagIsPacked |
        kAudioFormatFlagsNativeEndian;
    m_d->asbd.mFramesPerPacket = 1;
    m_d->asbd.mBitsPerChannel = sizeof(float) * 8;
    m_d->asbd.mBytesPerFrame = sizeof(float) * getChannelCount();
    m_d->asbd.mBytesPerPacket = sizeof(float) * getChannelCount();
	
/*
    cerr << "Client format contains:" << endl;
    for (int i = 0; i < sizeof(AudioStreamBasicDescription); ++i) {
        if (i % 8 == 0) cerr << endl;
        cerr << int(((char *)(&m_d->asbd))[i]) << " ";
    }
    cerr << endl;
*/
    m_d->err = ExtAudioFileSetProperty
	(m_d->file, kExtAudioFileProperty_ClientDataFormat,
         sizeof(AudioStreamBasicDescription), &m_d->asbd);
    
    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Error in setting client format: code " + codestr(m_d->err);
//        cerr << m_error << endl;
	ExtAudioFileDispose(m_d->file);
        throw FileOperationFailed(getPath(), "set client format");
    }

    // Initialise writes
    m_d->err = ExtAudioFileWriteAsync(m_d->file, 0, 0);
    
    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Error in initialising file writes: code " + codestr(m_d->err);
//        cerr << m_error << endl;
	ExtAudioFileDispose(m_d->file);
        throw FileOperationFailed(getPath(), "initialise file writes");
    }

    m_d->buffer.mNumberBuffers = 1;
    m_d->buffer.mBuffers[0].mNumberChannels = getChannelCount();
    m_d->buffer.mBuffers[0].mDataByteSize = 0;
    m_d->buffer.mBuffers[0].mData = 0;
}

CoreAudioWriteStream::~CoreAudioWriteStream()
{
    if (m_d->file) {
//        cerr << "CoreAudioWriteStream::~CoreAudioWriteStream: disposing" << endl;
	ExtAudioFileDispose(m_d->file);
    }
}

void
CoreAudioWriteStream::putInterleavedFrames(size_t count, const float *frames)
{
    if (count == 0) return;

    m_d->buffer.mBuffers[0].mDataByteSize =
        sizeof(float) * getChannelCount() * count;
    
    m_d->buffer.mBuffers[0].mData = (void *)frames;

    UInt32 framesToWrite = count;

    m_d->err = ExtAudioFileWrite(m_d->file, framesToWrite, &m_d->buffer);
    if (m_d->err) {
        m_error = "CoreAudioWriteStream: Error in encoder: code " + codestr(m_d->err);
//        cerr << m_error << endl;
        throw FileOperationFailed(getPath(), "encode");
    }
}

void
CoreAudioWriteStream::flush()
{
}

}

#endif
