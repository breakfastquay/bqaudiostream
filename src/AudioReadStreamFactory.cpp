/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_QUICKTIME
// Annoyingly, this doesn't like being included further down
#include <QuickTime/QuickTime.h>
#endif

#include "AudioReadStreamFactory.h"
#include "AudioReadStream.h"

#include "base/ThingFactory.h"
#include "base/Exceptions.h"
#include "system/Debug.h"

#include <QFileInfo>

#define DEBUG_AUDIO_READ_STREAM_FACTORY 1

namespace Turbot {

typedef ThingFactory<AudioReadStream, QString>
AudioReadStreamFactoryImpl;

AudioReadStream *
AudioReadStreamFactory::createReadStream(QString audioFileName)
{
    AudioReadStream *s = 0;

    QString extension = QFileInfo(audioFileName).suffix().toLower();

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

QStringList
AudioReadStreamFactory::getSupportedFileExtensions()
{
    return AudioReadStreamFactoryImpl::getInstance()->getTags();
}

bool
AudioReadStreamFactory::isExtensionSupportedFor(QString fileName)
{
    return getSupportedFileExtensions().contains
        (QFileInfo(fileName).suffix().toLower());
}

QString
AudioReadStreamFactory::getFileFilter()
{
    QStringList extensions = getSupportedFileExtensions();
    QString filter;
    foreach (QString ext, extensions) {
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
#include "QuickTimeReadStream.cpp"
#include "OggVorbisReadStream.cpp"
#include "DirectShowReadStream.cpp"
#include "CoreAudioReadStream.cpp"
#include "BasicMp3ReadStream.cpp"

#include "AudioStreamColumnBuilders.cpp"
