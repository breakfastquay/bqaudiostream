/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef BQ_AUDIO_READ_STREAM_FACTORY_H_
#define BQ_AUDIO_READ_STREAM_FACTORY_H_

#include <string>


namespace breakfastquay {

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
     * AudioReadStream::FileDRMProtected, InvalidFileFormat,
     * FileOperationFailed, or UnknownFileType.
     *
     * This function never returns NULL; it will always throw an
     * exception instead. (If there is simply no read stream
     * registered for the file extension, it will throw
     * UnknownFileType.)
     */
    static AudioReadStream *createReadStream(std::string fileName);

    /**
     * Return a list of the file extensions supported by registered
     * readers (e.g. "wav", "aiff", "mp3").
     */
    static std::vector<std::string> getSupportedFileExtensions();

    /**
     * Return true if the given extension (e.g. "wav") is supported by
     * a registered reader.
     */
    static bool isExtensionSupportedFor(std::string fileName);

    /**
     * Return a string containing the file extensions supported by
     * registered readers, in a format suitable for use as a file
     * dialog filter (e.g. "*.wav *.aiff *.mp3").
     */
    static std::string getFileFilter();
};

}

#endif
