/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifdef HAVE_QUICKTIME
// Annoyingly, this doesn't like being included further down
#include <QuickTime/QuickTime.h>
#endif

#include "AudioReadStreamFactory.h"
#include "AudioReadStream.h"

#include "base/ThingFactory.h"
#include "system/Debug.h"

#include <QFileInfo>

#define DEBUG_AUDIO_READ_STREAM_FACTORY 1

namespace Turbot {

typedef ThingFactory<AudioReadStream, QString>
AudioReadStreamFactoryImpl;

//template <>
//AudioReadStreamFactoryImpl *
//AudioReadStreamFactoryImpl::m_instance = 0;

AudioReadStream *
AudioReadStreamFactory::createReadStream(QString audioFileName)
{
    AudioReadStream *s = 0;

    QString extension = QFileInfo(audioFileName).suffix().toLower();

    AudioReadStreamFactoryImpl *f = AudioReadStreamFactoryImpl::getInstance();

    // Try to use a reader that has actually registered an interest in
    // this extension, first

    try {
        s = f->createFor(extension, audioFileName);
    } catch (...) {
    }

    if (s && s->isOK() && s->getError() == "") {
        return s;
    } else if (s) {
        std::cerr << "Error with recommended reader: \""
                  << s->getError() << "\""
                  << std::endl;
    }

    delete s;
    s = 0;

    // If that fails, try all readers in arbitrary order

    AudioReadStreamFactoryImpl::URISet uris = f->getURIs();

    for (AudioReadStreamFactoryImpl::URISet::const_iterator i = uris.begin();
         i != uris.end(); ++i) {

        try {
            s = f->create(*i, audioFileName);
        } catch (UnknownThingException) { }

        if (s && s->isOK() && s->getError() == "") {
            return s;
        }

        delete s;
        s = 0;
    }

    return 0;
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

