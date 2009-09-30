/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_WRITE_STREAM_FACTORY_H_
#define _TURBOT_AUDIO_WRITE_STREAM_FACTORY_H_

#include <QString>

namespace Turbot {

class AudioWriteStream;

class AudioWriteStreamFactory
{
public:
    static AudioWriteStream *createWriteStream(QString fileName,
                                               size_t channelCount,
                                               size_t sampleRate);
};

}

#endif
