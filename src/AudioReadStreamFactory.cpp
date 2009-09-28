/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioReadStreamFactory.h"
#include "WavFileReadStream.h"
#include "QuickTimeReadStream.h"
#include "DirectShowReadStream.h"

#include <iostream>
using std::cerr;
using std::endl;

#include "base/ThingFactory.h"

namespace Turbot {

// This is sort of half-way to being converted to a nice ThingFactory
// implementation, but only half-way

typedef ThingFactory<AudioReadStream, std::string>
AudioReadStreamFactoryImpl;

template <>
AudioReadStreamFactory *
AudioReadStreamFactoryImpl::m_instance = 0;

AudioReadStream *
AudioReadStreamFactory::createReadStream(std::string audioFileName)
{
    AudioReadStream *s = 0;

    AudioReadStreamFactoryImpl *f = AudioReadStreamFactoryImpl::getInstance();
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
    }

    return 0;
}

std::vector<std::string>
AudioReadStreamFactory::getSupportedFileExtensions()
{
    std::vector<std::string> extensions;
    std::vector<std::string> e;

#ifdef HAVE_LIBSNDFILE
    e = WavFileReadStream::getSupportedFileExtensions();
    for (int i = 0; i < e.size(); ++i) extensions.push_back(e[i]);
#endif

#ifdef HAVE_QUICKTIME
    e = QuickTimeReadStream::getSupportedFileExtensions();
    for (int i = 0; i < e.size(); ++i) extensions.push_back(e[i]);
#endif

#ifdef HAVE_DIRECTSHOW
    e = DirectShowReadStream::getSupportedFileExtensions();
    for (int i = 0; i < e.size(); ++i) extensions.push_back(e[i]);
#endif

    return extensions;
}

#ifndef HAVE_LIBSNDFILE
#ifndef HAVE_QUICKTIME
#ifndef HAVE_DIRECTSHOW
#pragma warning("No read stream implementation selected!")
#endif
#endif
#endif

}

