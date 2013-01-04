/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioStreamColumnReader.h"

#include "base/DataAreaCache.h"
#include "base/DataValueCache.h"
#include "base/TurbotTypes.h"

namespace Turbot {

class AudioStreamColumnReader::D
{
public:
    D(QString filename) :
	m_channels(0),
	m_columnCache(0),
	m_streamCache(0)
    {
    }

    ~D() {
    }

    void open();

    QString getFileName() const;

    Timebase getTimebase() const {
	return m_timebase;
    }

    int getChannelCount() const {
	return m_channels;
    }

    int getWidth() const;

    int getHeight() const {
	return m_timebase.getColumnSize() / 2 + 1;
    }

    void prepareColumn(int);

    bool getColumnPolarInterleaved
    (int x, int channel, turbot_sample_t *column);

    void close();


private:
    int m_channels;
    Timebase m_timebase;
    DataValueCache<turbot_sample_t> *m_columnCache;
    DataValueCache<float> *m_streamCache;
};

AudioStreamColumnReader::AudioStreamColumnReader(QString filename) :
    m_d(new D(filename))
{
}

AudioStreamColumnReader::~AudioStreamColumnReader()
{
    delete m_d;
}

void
AudioStreamColumnReader::open()
{
    m_d->open();
}

QString
AudioStreamColumnReader::getFileName() const
{
    return m_d->getFileName();
}

Timebase
AudioStreamColumnReader::getTimebase() const
{
    return m_d->getTimebase();
}

int
AudioStreamColumnReader::getChannelCount() const
{
    return m_d->getChannelCount();
}

int
AudioStreamColumnReader::getWidth() const
{
    return m_d->getWidth();
}

int
AudioStreamColumnReader::getHeight() const
{
    return m_d->getHeight();
}

void
AudioStreamColumnReader::prepareColumn(int col)
{
    m_d->prepareColumn(col);
}

bool
AudioStreamColumnReader::getColumnPolarInterleaved
(int x, int channel, turbot_sample_t *column)
{
    return m_d->getColumnPolarInterleaved(x, channel, column);
}

void
AudioStreamColumnReader::close()
{
    m_d->close();
}

}

