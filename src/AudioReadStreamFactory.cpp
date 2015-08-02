/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioReadStreamFactory.h"
#include "AudioReadStream.h"

#include <bqthingfactory/ThingFactory.h>

#define DEBUG_AUDIO_READ_STREAM_FACTORY 1

using namespace std;

namespace breakfastquay {

typedef ThingFactory<AudioReadStream, string>
AudioReadStreamFactoryImpl;

string
AudioReadStreamFactory::extensionOf(string audioFileName)
{
    string::size_type pos = audioFileName.rfind('.');
    if (pos == string::npos) return "";
    string ext;
    for (string::size_type i = pos; i < audioFileName.size(); ++i) {
        ext += tolower(audioFileName[i]);
    }
    return ext;
}

AudioReadStream *
AudioReadStreamFactory::createReadStream(string audioFileName)
{
    AudioReadStream *s = 0;

    string extension = extensionOf(audioFileName);

    AudioReadStreamFactoryImpl *f = AudioReadStreamFactoryImpl::getInstance();

    // Earlier versions of this code would first try to use a reader
    // that had actually registered an interest in this extension,
    // then fall back (if that failed) to trying every reader in
    // order. But we rely on extensions so much anyway, it's probably
    // more predictable always to use only the reader that has
    // registered the extension (if there is one).

    try {
        AudioReadStream *stream = f->createFor(extension, audioFileName);
        if (!stream) throw UnknownFileType(audioFileName);
        return stream;
    } catch (UnknownTagException) {
        throw UnknownFileType(audioFileName);
    }
}

vector<string>
AudioReadStreamFactory::getSupportedFileExtensions()
{
    return AudioReadStreamFactoryImpl::getInstance()->getTags();
}

bool
AudioReadStreamFactory::isExtensionSupportedFor(string fileName)
{
    return getSupportedFileExtensions().contains(extensionOf(fileName));
}

string
AudioReadStreamFactory::getFileFilter()
{
    vector<string> extensions = getSupportedFileExtensions();
    string filter;
    foreach (string ext, extensions) {
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
// separately in the project.

#include "WavFileReadStream.cpp"
#include "OggVorbisReadStream.cpp"
#include "MediaFoundationReadStream.cpp"
#include "CoreAudioReadStream.cpp"
#include "BasicMp3ReadStream.cpp"

