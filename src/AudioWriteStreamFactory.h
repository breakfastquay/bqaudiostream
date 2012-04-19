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
    /**
     * Create and return a write stream object for the given audio
     * file name, if possible. The audio file format will be deduced
     * from the file extension. If the file already exists, it will be
     * silently overwritten.
     *
     * May throw FailedToWriteFile, FileOperationFailed, or
     * UnknownFileType.
     *
     * This function never returns NULL; it will always throw an
     * exception instead. (If there is simply no write stream
     * registered for the file extension, it will throw
     * UnknownFileType.)
     */
    static AudioWriteStream *createWriteStreamE(QString fileName,
                                               size_t channelCount,
                                               size_t sampleRate);

    static QStringList getSupportedFileExtensions();

    static bool isExtensionSupportedFor(QString fileName);
};

}

#endif
