/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioStreamColumnReader.h"

#include "base/DataAreaCache.h"
#include "base/DataValueCache.h"
#include "base/TurbotTypes.h"
#include "base/Exceptions.h"

#include "AudioReadStream.h"
#include "AudioReadStreamFactory.h"

#include "musicdb/MusicDatabase.h"

#include "dsp/Window.h"
#include "dsp/FFT.h"

namespace Turbot {

class AudioStreamColumnReader::D
{
public:
    D(QString filename) :
	m_filename(filename),
	m_stream(0),
	m_channels(0),
	m_columnCache(0),
	m_streamCache(0),
        m_streamCacheColumnNo(-1)
    {
    }

    ~D() {
        if (m_stream) close();
    }

    void open() {
	if (m_stream) {
	    throw PreconditionFailed("AudioStreamColumnReader already open");
	}

        m_id = MusicDatabase::getInstance()->getID(m_filename);

	m_stream = AudioReadStreamFactory::createReadStream(m_filename);
	m_channels = m_stream->getChannelCount();
	m_timebase = Timebase(m_stream->getSampleRate(),
			      DefaultColumnSize,
			      DefaultHopSize);

        m_window = new Window<float>(HanningWindow, DefaultColumnSize);
        m_fft = new FFT(DefaultColumnSize);

        // Somewhat arbitrary cache size of 8 columns across all
        // channels. We need to allow backward seeks of at least 1 or
        // 2 columns for Region to operate.
	m_columnCache = new DataValueCache<turbot_sample_t>
	    (8 * m_channels * getHeight());

        // Stream cache is exactly one FFT-input frame in size,
        // and we read a hop then push it on the end at each step
        m_streamCache = allocate_channels<float>
            (m_channels, m_timebase.getColumnSize());

        m_streamCacheColumnNo = -1;
    }


    QString getFileName() const {
	return m_filename;
    }

    Timebase getTimebase() const {
	return m_timebase;
    }

    int getChannelCount() const {
	return m_channels;
    }

    int getWidth() const {
        int frames = MusicDatabase::getInstance()->getFrameCount(m_id);
        int hop = m_timebase.getHop();
        int sz = m_timebase.getColumnSize();
        return int(ceil(double(frames + (sz - hop)) / hop));
    }

    int getHeight() const {
	return m_timebase.getColumnSize() / 2 + 1;
    }

    void prepareColumn(int x) {
        //!!!
    }

    bool getColumnPolarInterleaved
    (int x, int channel, turbot_sample_t *column) {
        return false; //!!!
    }

    void close() {
        deallocate_channels(m_streamCache, m_channels);
	delete m_columnCache;
	delete m_stream;
        delete m_window;
        delete m_fft;
	m_stream = 0;
    }

private:
    void processColumn() {
        // Read one hop
        int hop = m_timebase.getHop();
        int sz = m_timebase.getColumnSize();
        float *iframes = allocate<float>(hop * m_channels);
        int read = m_stream->getInterleavedFrames(hop, iframes);
        if (read < hop) {
            //... indicate end of stream somehow
        } else {
            for (int c = 0; c < m_channels; ++c) {
                v_move(m_streamCache[c], m_streamCache[c] + hop, sz - hop);
                for (int i = 0; i < hop; ++i) {
                    m_streamCache[c][sz - hop + i] = iframes[i * m_channels + c];
                }
            }
            ++m_streamCacheColumnNo;
            processColumnFromStreamCache();
        }
        deallocate(iframes);
    }

    void processColumnFromStreamCache() {

        int columnNo = m_streamCacheColumnNo;
        int sz = m_timebase.getColumnSize();
        int hs1 = sz/2 + 1;
        int colSize = hs1 * 2;
        turbot_sample_t *columns = allocate<turbot_sample_t>
            (colSize * m_channels);

        float *in = allocate<float>(sz);
        float *magOut = allocate<float>(hs1);
        float *phaseOut = allocate<float>(hs1);

        for (int ch = 0; ch < m_channels; ++ch) {
            v_copy(in, m_streamCache[ch], sz);
            m_window->cut(in);
            v_fftshift(in, sz);
            m_fft->forwardPolar(in, magOut, phaseOut);
            for (int i = 0; i < hs1; ++i) {
                columns[colSize * ch + 2*i] = magOut[i];
                columns[colSize * ch + 2*i + 1] = phaseOut[i];
            }
        }

        m_columnCache->update(colSize * columnNo * m_channels, columns,
                              colSize * m_channels);

        deallocate(in);
        deallocate(magOut);
        deallocate(phaseOut);
    }

    QString m_filename;

    AudioReadStream *m_stream;

    int m_channels;
    Timebase m_timebase;

    Window<float> *m_window;
    FFT *m_fft;

    DataValueCache<turbot_sample_t> *m_columnCache; // columns interleaved

    float **m_streamCache; // per channel
    int m_streamCacheColumnNo;

    MusicDatabase::ID m_id;
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

