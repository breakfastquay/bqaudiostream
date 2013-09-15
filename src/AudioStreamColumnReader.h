/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_STREAM_COLUMN_READER_H_
#define _TURBOT_AUDIO_STREAM_COLUMN_READER_H_

#include "process/ChunkColumnReader.h"
#include "process/ChunkMetadataReader.h"

namespace Turbot {

/**
 * AudioStreamColumnReader is an adapter that provides data from an
 * AudioReadStream in the format expected by ChunkColumnReader.
 *
 * For performance reasons, neither getHumanOnset nor getPitchValue
 * are implemented.
 *
 * Reentrant. Not thread-safe; not RT-safe.
 */
class AudioStreamColumnReader : public QObject,
                                public ChunkColumnReader,
                                public ChunkMetadataReader
{
    Q_OBJECT

public:
    /**
     * Create a new AudioStreamColumnReader configured to read from
     * the given filename.
     */
    AudioStreamColumnReader(QString filename);

    ~AudioStreamColumnReader();

    /**
     * Must be called before open()
     */
    virtual void setRetrievalSampleRate(int rate);

    /**
     * Open the file. May throw any of the exceptions thrown by
     * AudioReadStreamFactory::createReadStream, or PreconditionFailed
     * if the AudioStreamColumnReader is not in a state suitable for
     * calling open() (e.g. if open() has already been called).
     */
    virtual void open();

    virtual QString getFileName() const;

    virtual Timebase getTimebase() const;
    virtual int getChannelCount() const;
    virtual int getWidth() const;
    virtual int getHeight() const;

    virtual void prepareColumn(int);

    virtual bool getColumnPolarInterleaved
    (int x, int channel, turbot_sample_t *column);

    virtual int getSupportedMetadata() const;
    virtual int getDefaultMetadata() const; // Not all calculated by default
    virtual void setRequiredMetadata(int);

    virtual bool getPhaseSync(int col);
    virtual bool getHumanOnset(int col); // Not implemented

    virtual turbot_sample_t getAudioCurveValue(int col); 
    virtual turbot_sample_t getColumnUniquePower(int col, int channel); 
    virtual turbot_sample_t getColumnTotalPower(int col, int channel); 
    virtual turbot_sample_t getPitchValue(int col, turbot_sample_t &confidence);

    virtual void close();

private:
    class D;
    D *m_d;

    AudioStreamColumnReader(const AudioStreamColumnReader &); // not provided
    AudioStreamColumnReader &operator=(const AudioStreamColumnReader &); // not provided

    static ChunkColumnReaderBuilder<AudioStreamColumnReader> m_colBuilder;
    static ChunkMetadataReaderBuilder<AudioStreamColumnReader> m_metaBuilder;
};

}

#endif
