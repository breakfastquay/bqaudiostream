/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioWriteStreamFactory.h"
#include "WavFileWriteStream.h"
#include "SimpleWavFileWriteStream.h"
/*
#include "QuickTimeWriteStream.h"
#include "DirectShowWriteStream.h"
*/

#include <iostream>
using std::cerr;
using std::endl;

namespace Turbot {

AudioFileWriteStream *
AudioWriteStreamFactory::createWriteStream(std::string audioFileName,
                                           size_t channelCount,
                                           size_t sampleRate)
{
    AudioFileWriteStream *s = 0;

    //!!! At the moment all we handle is WAV files, but we'll have to
    //!!! have some way to determine whether a write stream can
    //!!! convert to a particular type!  The SimpleWavFileWriteStream
    //!!! will always succeed, regardless of the file's extension --
    //!!! it'll just always write WAV.

#ifdef HAVE_LIBSNDFILE
    if (!s || !s->isOK()) {
        delete s;
        s = new WavFileWriteStream(audioFileName, channelCount, sampleRate);
    }
#endif

/*
#ifdef HAVE_QUICKTIME
    if (!s || !s->isOK()) {
        delete s;
        s = new QuickTimeWriteStream(audioFileName, channelCount, sampleRate);
    }
#endif
*/
/*
#ifdef HAVE_DIRECTSHOW
    if (!s || !s->isOK()) {
        delete s;
        s = new DirectShowWriteStream(audioFileName, channelCount, sampleRate);
    }
#endif
*/

    if (!s || !s->isOK()) {
        delete s;
        s = new SimpleWavFileWriteStream(audioFileName, channelCount, sampleRate);
    }

    if (!s || !s->isOK()) {
        delete s;
        return 0;
    }

    return s;
}

#ifndef HAVE_LIBSNDFILE
//#ifndef HAVE_QUICKTIME
//#ifndef HAVE_DIRECTSHOW
#pragma warning("No read stream implementation selected!")
//#endif
//#endif
#endif

}

