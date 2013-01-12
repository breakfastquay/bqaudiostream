/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#ifndef _TURBOT_AUDIO_STREAM_COLUMN_READER_H_
#define _TURBOT_AUDIO_STREAM_COLUMN_READER_H_

#include "process/RegionColumnReader.h"
#include "process/RegionMetadataReader.h"

namespace Turbot {

/**
 * AudioStreamColumnReader is an adapter that provides data from an
 * AudioReadStream in the format expected by RegionColumnReader.
 *
 * Reentrant. Not thread-safe; not RT-safe.
 */
class AudioStreamColumnReader : virtual public RegionColumnReader,
                                virtual public RegionMetadataReader
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

    virtual bool getPhaseSync(int col);
    virtual bool getHumanOnset(int col);

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

    static RegionColumnReaderBuilder<AudioStreamColumnReader> m_colBuilder;
    static RegionMetadataReaderBuilder<AudioStreamColumnReader> m_metaBuilder;
};

}

#endif
