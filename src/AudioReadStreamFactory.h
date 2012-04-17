/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_READ_STREAM_FACTORY_H_
#define _TURBOT_AUDIO_READ_STREAM_FACTORY_H_

#include <QString>
#include <QStringList>

namespace Turbot {

class AudioReadStream;

class AudioReadStreamFactory
{
public:
    /**
     * Create and return a read stream object for the given audio file
     * name, if possible. The audio format will be deduced from the
     * file extension.
     *
     * May throw FileNotFound, FileOpenFailed,
     * AudioReadStream::FileDRMProtected, InvalidFileFormat, or
     * FileOperationFailed. May also return NULL if the file extension
     * is unrecognised.
     */
    static AudioReadStream *createReadStreamE(QString fileName);

    static QStringList getSupportedFileExtensions();

    static bool isExtensionSupportedFor(QString fileName);
};

}

#endif
