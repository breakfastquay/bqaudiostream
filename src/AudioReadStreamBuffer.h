/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_READ_STREAM_BUFFER_H_
#define _TURBOT_AUDIO_READ_STREAM_BUFFER_H_

#include <QString>

namespace Turbot {

/**
 * AudioReadStreamBuffer opens and reads from a non-seekable audio
 * file stream, buffering up a predefined number of frames and
 * refilling the buffer as frames are consumed.
 *
 * Reading frames that have already been buffered is wait-free and the
 * buffer is refilled asynchronously. There is no guarantee that any
 * given number of frames can be read from the buffer at any moment;
 * it depends on the refill progress.
 *
 * Note that AudioReadStreamBuffer may hold the file descriptor for
 * the underlying audio file open throughout its lifespan.
 */
class AudioReadStreamBuffer
{
public:
    /**
     * Create and return a buffered reader for the given audio file,
     * if possible, with a buffer size of the given duration in
     * seconds. The audio format will be deduced from the file
     * extension. The buffer will be filled from the start of the file
     * stream before the constructor returns, so it may take some
     * time.
     *
     * If retrieveAtRate is non-zero, the audio file will be resampled
     * to that rate during reading (if necessary).
     *
     * May throw FileNotFound, FileOpenFailed,
     * AudioReadStream::FileDRMProtected, InvalidFileFormat,
     * FileOperationFailed, or UnknownFileType.
     */
    AudioReadStreamBuffer(QString fileName,
			  double bufferLengthSecs,
			  int retrieveAtRate = 0);
    
    /**
     * Destroy the buffered reader and close the file.
     */
    ~AudioReadStreamBuffer();

    int getChannelCount() const;
    int getSampleRate() const;

    /**
     * Throw away any buffered frames and return to the start of the
     * stream. This operation will block on refilling the buffer, and
     * may take some time.
     */
    void rewind();

    /** 
     * Return the number of frames available for reading from the
     * buffer.
     */
    int getAvailableFrameCount() const; //[RT]

    /**
     * Read the given number of frames, if available. Return the
     * number actually read. The frames argument must point to
     * getChannelCount() arrays capable of holding count elements
     * each.
     */
    int getFrames(int count, float **frames); //[RT]

private:
    class D;
    D *m_d;
};

}

#endif
