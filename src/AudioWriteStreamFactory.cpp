/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioWriteStreamFactory.h"
#include "AudioWriteStream.h"

#include "base/ThingFactory.h"
#include "base/Exceptions.h"

#include <QFileInfo>

namespace Turbot {

typedef ThingFactory<AudioWriteStream, AudioWriteStream::Target>
AudioWriteStreamFactoryImpl;

//template <>
//AudioWriteStreamFactoryImpl *
//AudioWriteStreamFactoryImpl::m_instance = 0;

AudioWriteStream *
AudioWriteStreamFactory::createWriteStream(QString audioFileName,
                                           size_t channelCount,
                                           size_t sampleRate)
{
    AudioWriteStream *s = 0;

    QString extension = QFileInfo(audioFileName).suffix().toLower();

    AudioWriteStream::Target target(audioFileName, channelCount, sampleRate);

    AudioWriteStreamFactoryImpl *f = AudioWriteStreamFactoryImpl::getInstance();

    AudioWriteStream *stream = f->createFor(extension, target);
    if (!stream) throw UnknownFileType(audioFileName);
    return stream;
}

QStringList
AudioWriteStreamFactory::getSupportedFileExtensions()
{
    return AudioWriteStreamFactoryImpl::getInstance()->getTags();
}

QString
AudioWriteStreamFactory::getDefaultUncompressedFileExtension()
{
    QStringList candidates;
    candidates << "wav" << "aiff";
    QStringList supported = getSupportedFileExtensions();
    foreach (QString ext, candidates) {
        if (supported.contains(ext)) return ext;
    }
    return "";
}

QString
AudioWriteStreamFactory::getDefaultLossyFileExtension()
{
    QStringList candidates;
    candidates << "mp3" << "m4a" << "ogg" << "oga";
    QStringList supported = getSupportedFileExtensions();
    foreach (QString ext, candidates) {
        if (supported.contains(ext)) return ext;
    }
    return "";
}

bool
AudioWriteStreamFactory::isExtensionSupportedFor(QString fileName)
{
    return getSupportedFileExtensions().contains
        (QFileInfo(fileName).suffix().toLower());
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

#include "WavFileWriteStream.cpp"
#include "SimpleWavFileWriteStream.cpp"
#include "CoreAudioWriteStream.cpp"

