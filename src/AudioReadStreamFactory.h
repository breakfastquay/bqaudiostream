/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_READ_STREAM_FACTORY_H_
#define _TURBOT_AUDIO_READ_STREAM_FACTORY_H_

#include <string>
#include <vector>

namespace Turbot {

class AudioReadStream;

class AudioReadStreamFactory
{
public:
    // May throw FileOpenFailed or AudioReadStream::FileDRMProtected,
    // or simply return NULL, for failure
    static AudioReadStream *createReadStream(std::string fileName);

    static std::vector<std::string> getSupportedFileExtensions();
};

}

#endif
