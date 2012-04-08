/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_DIRECTORY_INDEX_H_
#define _TURBOT_AUDIO_DIRECTORY_INDEX_H_

#include <QObject>
#include <QString>

namespace Turbot {

class AudioDirectoryIndex : public QObject
{
    Q_OBJECT

public:
    /**
     * Start reading the given directory and indexing the audio files
     * in it. Indexing involves opening each file with
     * AudioReadStreamFactory to determine whether it is actually of a
     * supported type.
     *
     * Indexing happens asynchronously; the full list is not available
     * until complete() has been emitted (or wait() has been called
     * and returned) but incremental values may be retrieved at any
     * time.
     */
    AudioDirectoryIndex(QString directoryPath /*!!! not yet: , bool recursive */);

    /**
     * Return the number of files indexed that can be opened by
     * AudioReadStreamFactory. If indexing is not yet complete, this
     * will return the number indexed so far.
     */
    int getGoodFileCount() const;

    /**
     * Return the number of files indexed that cannot be opened by
     * AudioReadStreamFactory because their file types are not
     * supported. If indexing is not yet complete, this will return
     * the number indexed so far.
     */
    int getUnsupportedFileCount() const;

    /**
     * Return the number of files indexed that cannot be opened by
     * AudioReadStreamFactory because they are DRM-protected. If
     * indexing is not yet complete, this will return the number
     * indexed so far.
     */
    int getProtectedFileCount() const;

    /**
     * Return a list of file paths for audio files indexed that can be
     * opened by AudioReadStreamFactory. If indexing is not yet
     * complete, this will return the files indexed so far.
     */
    QStringList getGoodFiles() const;

    /**
     * Return a list of file paths for audio files indexed that cannot
     * be opened by AudioReadStreamFactory because their file types
     * are not supported. If indexing is not yet complete, this will
     * return the files indexed so far.
     */
    QStringList getUnsupportedFiles() const;

    /**
     * Return a list of file paths for audio files indexed that cannot
     * be opened by AudioReadStreamFactory because they are
     * DRM-protected. If indexing is not yet complete, this will
     * return the files indexed so far.
     */
    QStringList getProtectedFiles() const;

    /**
     * Block and wait until indexing is complete.
     */
    void wait();
    
    /**
     * Return true if indexing is complete (see also complete()).
     */
    bool isComplete() const;
    
    /**
     * Return the approximate percentage of files indexed so far, for
     * progress reporting purposes (see also progressUpdated()).
     */
    int getCompletion() const;

signals:
    /**
     * Emitted regularly during indexing with the percentage of files
     * indexed so far, for reporting purposes.
     */
    void progressUpdated(int percent);
    
    /**
     * Emitted when indexing is complete.
     */
    void complete();
};

}

#endif
