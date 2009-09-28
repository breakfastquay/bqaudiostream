/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_READ_STREAM_FACTORY_H_
#define _TURBOT_AUDIO_READ_STREAM_FACTORY_H_

#include <string>
#include <vector>

#include "base/ThingFactory.h"

namespace Turbot {

class AudioFileReadStream;

class AudioReadStreamFactory
{
public:
    // May throw FileOpenFailed or AudioFileReadStream::FileDRMProtected,
    // or simply return NULL, for failure
    static AudioFileReadStream *createReadStream(std::string fileName);

    static std::vector<std::string> getSupportedFileExtensions();
};

typedef ThingFactory<AudioFileReadStream, std::string>
AudioFileReadStreamFactory;

}

#endif
