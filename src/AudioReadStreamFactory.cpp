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

#include "../bqaudiostream/AudioReadStreamFactory.h"
#include "../bqaudiostream/AudioReadStream.h"
#include "../bqaudiostream/Exceptions.h"

#include <bqthingfactory/ThingFactory.h>

#define DEBUG_AUDIO_READ_STREAM_FACTORY 1

namespace breakfastquay {

typedef ThingFactory<AudioReadStream, std::string>
AudioReadStreamFactoryImpl;

std::string
AudioReadStreamFactory::extensionOf(std::string audioFileName)
{
    std::string::size_type pos = audioFileName.rfind('.');
    if (pos == std::string::npos) return "";
    std::string ext;
    for (std::string::size_type i = pos + 1; i < audioFileName.size(); ++i) {
        ext += (char)tolower((unsigned char)audioFileName[i]);
    }
    return ext;
}

AudioReadStream *
AudioReadStreamFactory::createReadStream(std::string audioFileName)
{
    std::string extension = extensionOf(audioFileName);

    AudioReadStreamFactoryImpl *f = AudioReadStreamFactoryImpl::getInstance();

    // Earlier versions of this code would first try to use a reader
    // that had actually registered an interest in this extension,
    // then fall back (if that failed) to trying every reader in
    // order. But we rely on extensions so much anyway, it's probably
    // more predictable always to use only the reader that has
    // registered the extension (if there is one).

    if (extension == "") {
        // We explicitly support extension-less files so long as they
        // are RIFF/WAVE format. (This is in order to support
        // programmatically generated temporary files created with
        // e.g. mkstemp.)
        extension = "wav";
    }
    
    try {
        AudioReadStream *stream = f->createFor(extension, audioFileName);
        if (!stream) throw UnknownFileType(audioFileName);
        return stream;
    } catch (const UnknownTagException &) {
        throw UnknownFileType(audioFileName);
    }
}

std::vector<std::string>
AudioReadStreamFactory::getSupportedFileExtensions()
{
    return AudioReadStreamFactoryImpl::getInstance()->getTags();
}

bool
AudioReadStreamFactory::isExtensionSupportedFor(std::string fileName)
{
    std::vector<std::string> supported = getSupportedFileExtensions();
    std::set<std::string> sset(supported.begin(), supported.end());
    return sset.find(extensionOf(fileName)) != sset.end();
}

std::string
AudioReadStreamFactory::getFileFilter()
{
    std::vector<std::string> extensions = getSupportedFileExtensions();
    std::string filter;
    for (size_t i = 0; i < extensions.size(); ++i) {
        std::string ext = extensions[i];
        if (filter != "") filter += " ";
        filter += "*." + ext;
    }
    return filter;
}

}

// We rather eccentrically include the C++ files here, not the
// headers.  This file actually doesn't need any includes in order to
// compile, but we are building it into a static archive, from which
// only those object files that are referenced in the code that uses
// the archive will be extracted for linkage.  Since no code refers
// directly to the stream implementations (they are self-registering),
// this means they will not be linked in.  So we include them directly
// into this object file instead, and it's not necessary to build them
// separately in the project. In each case the code is completely
// #ifdef'd out if the implementation is not selected, so there is no
// overhead.

// Note that our ThingFactory always uses the first builder registered
// for a given tag. Within a single source file (as this is), the
// builders are guaranteed to be registered in lexical order. So we
// should put the desirable readers first and the iffy ones after.

// SimpleWavFileReadStream reads most WAV files. It's much more
// limited than the libsndfile-based WavFileReadStream, but it doesn't
// lack any features we actually use, and it includes optional
// incremental reading (read-during-write) which the other doesn't, so
// we have it first. One of these two must also come before the other
// general platform frameworks because we don't currently have seek
// support in those
#include "SimpleWavFileReadStream.cpp"

// WavFileReadStream uses libsndfile, which is mostly trustworthy
#include "WavFileReadStream.cpp"

// OggVorbisReadStream uses the official libraries, which ought to be good
#include "OggVorbisReadStream.cpp"

// OpusReadStream uses the official libraries, which ought to be good
#include "OpusReadStream.cpp"

// CoreAudioReadStream should be pretty good
#include "CoreAudioReadStream.cpp"

// MediaFoundationReadStream should be ok and at least should be what
// people using Windows expect
#include "MediaFoundationReadStream.cpp"

// MiniMP3ReadStream seems decent but lacks ID3 support, so in
// practice is not as good as the platform frameworks
#include "MiniMP3ReadStream.cpp"

