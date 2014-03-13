/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioStreamColumnReader.h"

#include "base/DataAreaCache.h"
#include "base/DataValueCache.h"
#include "base/TurbotTypes.h"
#include "base/Exceptions.h"

#include "system/sysutils.h"

#include "AudioReadStream.h"
#include "AudioReadStreamFactory.h"

#include "musicdb/MusicDatabase.h"

#include "dsp/Window.h"
#include "bqfft/FFT.h"

#include "audiocurves/CompoundAudioCurve.h"
#include "audiocurves/CepstralPitchCurve.h"

#include <vector>

#include <cassert>

using namespace breakfastquay;

#define DEBUG_AUDIO_STREAM_COLUMN_READER 1
//#define DEBUG_AUDIO_STREAM_COLUMN_READER_PROCESS 1

using std::vector;
using std::cerr;
using std::endl;

namespace Turbot {

class AudioStreamColumnReader::D
{
    enum ColumnLocation {
        InCache,
        LeftOfCache,
        NearRightOfCache,
        FarRightOfCache,
        OutsideFile
    };

public:
    D(QString filename) :
	m_filename(filename),
	m_stream(0),
	m_channels(0),
	m_columnCache(0),
        m_metadataFlags(AllMetadata),
        m_audioCurve(0),
        m_pitchCurve(0),
	m_streamCache(0),
        m_streamCacheColumnNo(-1),
        m_retrievalRate(0)
    {
    }

    ~D() {
        if (m_stream) close();
    }

    void setRetrievalSampleRate(int rate) {
        m_retrievalRate = rate;
    }

    void open() {
	if (m_stream) {
	    throw PreconditionFailed("AudioStreamColumnReader already open");
	}

        m_id = MusicDatabase::getInstance()->getID(m_filename);

	m_stream = AudioReadStreamFactory::createReadStream(m_filename);
	m_channels = m_stream->getChannelCount();

        int rate = m_retrievalRate;
        if (rate == 0) rate = m_stream->getSampleRate();
        else if (rate != m_stream->getSampleRate()) {
            m_stream->setRetrievalSampleRate(rate);
        }

        int sz = DefaultColumnSize;
        int hop = DefaultHopSize;

	m_timebase = Timebase(rate, sz, hop);
        m_window = new Window<turbot_sample_t>(HanningWindow, sz);
        m_fft = new FFT(sz);

        // Somewhat arbitrary cache size of 8 columns across all
        // channels. We need to allow backward seeks of at least 1 or
        // 2 columns for Region to operate.
	m_columnCache = new DataValueCache<turbot_sample_t>
	    (8 * m_channels * getHeight());

        // Stream cache is exactly one FFT-input frame in size,
        // and we read a hop then push it on the end at each step
        m_streamCache = allocate_channels<float>(m_channels, sz);
        v_zero_channels(m_streamCache, m_channels, sz);

        m_streamCacheColumnNo = -1;

        m_audioCurve = new CompoundAudioCurve
            (CompoundAudioCurve::Parameters(rate, sz));

        m_pitchCurve = new CepstralPitchCurve
            (CepstralPitchCurve::Parameters(rate, sz));
    }

    void close() {
        deallocate_channels(m_streamCache, m_channels);
        delete m_audioCurve;
        delete m_pitchCurve;
	delete m_columnCache;
	delete m_stream;
        delete m_window;
        delete m_fft;
	m_stream = 0;
    }

    void rewind() {
        delete m_stream;
	m_stream = AudioReadStreamFactory::createReadStream(m_filename);
        v_zero_channels(m_streamCache, m_channels, DefaultColumnSize);
        m_streamCacheColumnNo = -1;
        m_columnCache->clear();
        // leave m_df unchanged
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

    bool prepareColumn(int x) {
#ifdef DEBUG_AUDIO_STREAM_COLUMN_READER_PROCESS
        std::cerr << "AudioStreamColumnReader[" << this << "]::prepareColumn(" << x << ")" << std::endl;
#endif
        ColumnLocation loc = findColumnRelativeToCache(x);
        if (loc == OutsideFile) {
            return false;
        } else if (loc == LeftOfCache) {
#ifdef DEBUG_AUDIO_STREAM_COLUMN_READER
            std::cerr << "JUMP to " << x << std::endl;
#endif
            rewind();
        }

        //!!!
        if (findColumnRelativeToCache(x) == FarRightOfCache) {
#ifdef DEBUG_AUDIO_STREAM_COLUMN_READER
            std::cerr << "SKIP to " << x << std::endl;
#endif
        }

        while (findColumnRelativeToCache(x) == FarRightOfCache &&
               m_streamCacheColumnNo + 2 < x) {
            processColumnMetadataOnly();
        }
        while (findColumnRelativeToCache(x) == NearRightOfCache ||
               findColumnRelativeToCache(x) == FarRightOfCache) {
#ifdef DEBUG_AUDIO_STREAM_COLUMN_READER
            std::cerr << "HOP to " << x << std::endl;
#endif
            processColumn();
        }
        return true;
    }

    bool getColumnPolarInterleaved
    (int x, int channel, turbot_sample_t *column) {
        if (!prepareColumn(x)) return false;
        retrieveColumnFromCache(x, channel, column);
        return true;
    }

    int getSupportedMetadata() const {
        return PhaseSync | AudioCurve | UniquePower | TotalPower | Pitch;
    }

    int getDefaultMetadata() const {
        return PhaseSync | AudioCurve | UniquePower;
    }

    void setRequiredMetadata(int mflags) {
        m_metadataFlags = mflags;

        // Must have AudioCurve is PhaseSync is requested, we use it
        // to calculate PhaseSync
        if (m_metadataFlags & PhaseSync) {
            m_metadataFlags |= AudioCurve;
        }
    }

    bool getPhaseSync(int x) {
        if (!(m_metadataFlags & PhaseSync)) return false;
        if (x < 0) return false;
        turbot_sample_t prev = (x == 0 ? 0 : getAudioCurveValue(x-1));
        turbot_sample_t curr = getAudioCurveValue(x);
        const turbot_sample_t transientThreshold = 0.35;
        if (curr > prev * 1.1 && curr > transientThreshold && !getPhaseSync(x-1)) {
            return true;
        }
        return false;
    }

    bool getHumanOnset(int x) {
        if (!(m_metadataFlags & HumanOnset)) return false;
        if (!prepareColumn(x)) return false;
        return false; //!!!
    }

    turbot_sample_t getAudioCurveValue(int x) {
        if (!(m_metadataFlags & AudioCurve)) return 0;
        if (x >= curvesLength() && !prepareColumn(x)) return 0;
        assert(x >= 0 && x < curvesLength());
        return m_df[x];
    }

    turbot_sample_t getColumnUniquePower(int x, int channel) {
        if (!(m_metadataFlags & UniquePower)) return 0;
        if (x >= curvesLength() && !prepareColumn(x)) return 0;
        assert(x >= 0 && x < curvesLength());
        int ix = m_channels * x + channel;
        assert(ix < (int)m_uniquePowers.size());
        return m_uniquePowers[ix];
    }

    turbot_sample_t getColumnTotalPower(int x, int channel) {
        if (!(m_metadataFlags & TotalPower)) return 0;
        if (x >= curvesLength() && !prepareColumn(x)) return 0;
        assert(x >= 0 && x < curvesLength());
        int ix = m_channels * x + channel;
        assert(ix < (int)m_totalPowers.size());
        return m_totalPowers[ix];
    }

    turbot_sample_t getPitchValue(int x, turbot_sample_t &confidence) {
        if (!(m_metadataFlags & Pitch)) {
            confidence = 0; return 0;
        }
        if (x >= curvesLength() && !prepareColumn(x)) {
            confidence = 0; return 0; 
        }
        assert(x >= 0 && x < curvesLength());
        confidence = m_pitchConfidence[x];
        return m_pitch[x];
    }

private:
    int singleChannelColumnSize() {
        int sz = m_timebase.getColumnSize();
        int hs1 = sz/2 + 1;
        int colSize = hs1 * 2;
        return colSize;
    }

    int offsetForColumn(int x, int channel) {
        int colSize = singleChannelColumnSize();
        int ix = x * colSize * m_channels + channel * colSize;
        return ix;
    }

    ColumnLocation findColumnRelativeToCache(int x) {

        if (x < 0 || x >= getWidth()) {
            return OutsideFile;
        }

        int colSize = singleChannelColumnSize();
        int ix = offsetForColumn(x, 0);

        int left = m_columnCache->offset();
        int right = m_columnCache->offset() + m_columnCache->size();

        if (ix < left) {
            return LeftOfCache;
        }
        if (ix + colSize * m_channels > right) {
            if (ix > right + 7 * colSize * m_channels) {
                return FarRightOfCache;
            } else {
                return NearRightOfCache;
            }
        }

        return InCache;
    }

    void retrieveColumnFromCache(int x, int channel, turbot_sample_t *column) {
        int colSize = singleChannelColumnSize();
        if (findColumnRelativeToCache(x) != InCache) {
            cerr << "retrieveColumnFromCache: column " << x << " not in cache (starts " << m_columnCache->offset() / (colSize * m_channels) << " of size " << m_columnCache->size() / (colSize * m_channels) << ")" << endl;
            throw PreconditionFailed("retrieveColumnFromCache: column not in cache");
        }
        int ix = offsetForColumn(x, channel);
        const turbot_sample_t *data = m_columnCache->data();
        v_copy(column, data + (ix - m_columnCache->offset()), colSize);
    }

    void readToStreamCache() {

        // Read one hop

        int hop = m_timebase.getHop();
        int sz = m_timebase.getColumnSize();

        float *iframes = allocate<float>(hop * m_channels);

        int read = m_stream->getInterleavedFrames(hop, iframes);
        if (read < hop) {
            for (int c = 0; c < m_channels; ++c) {
                for (int i = read; i < hop; ++i) {
                    iframes[i * m_channels + c] = 0.f;
                }
            }
        }

        for (int c = 0; c < m_channels; ++c) {
            v_move(m_streamCache[c], m_streamCache[c] + hop, sz - hop);
            for (int i = 0; i < hop; ++i) {
                m_streamCache[c][sz - hop + i] = iframes[i * m_channels + c];
            }
        }
        ++m_streamCacheColumnNo;

        deallocate(iframes);
    }

    void processColumn() {
        readToStreamCache();
        processColumnFromStreamCache();
    }

    void processColumnMetadataOnly() {
        readToStreamCache();
        processColumnFromStreamCacheMetadataOnly();
    }

    void processColumnFromStreamCache() {

        int columnNo = m_streamCacheColumnNo;

        if (columnNo > curvesLength()) {
            cerr << "processColumnFromStreamCache: column " << columnNo << " found, but audio curve only reaches " << int(curvesLength())-1 << " -- can't append (no way to fill the gap)" << endl;
            throw PreconditionFailed("processColumnFromStreamCache: column to right of audio curve (would leave a gap)");
        }

        int sz = m_timebase.getColumnSize();
        int hs1 = sz/2 + 1;
        int colSize = hs1 * 2;
        turbot_sample_t *columns = allocate<turbot_sample_t>
            (colSize * m_channels);

        turbot_sample_t *in = allocate<turbot_sample_t>(sz);
        turbot_sample_t *magOut = allocate<turbot_sample_t>(hs1);
        turbot_sample_t *phaseOut = allocate<turbot_sample_t>(hs1);

        turbot_sample_t *magSum = 0;

        if (m_channels > 0 && columnNo == curvesLength()) {
            // We will be appending a metadata calculation and we need
            // to mix down for it
            magSum = allocate<turbot_sample_t>(hs1);
            v_zero(magSum, hs1);
        }
        
        for (int ch = 0; ch < m_channels; ++ch) {

            v_convert(in, m_streamCache[ch], sz);

            addUniquePowerAndPeak(in);

            m_window->cut(in);
            v_fftshift(in, sz);

            addTotalPower(in);

            m_fft->forwardPolar(in, magOut, phaseOut);

            for (int i = 0; i < hs1; ++i) {

#ifdef DEBUG_AUDIO_STREAM_COLUMN_READER_PROCESS
                cerr << "columns[" << colSize * ch + 2*i << "] <- magOut[" << i << "] = " << magOut[i] << endl;
#endif

                columns[colSize * ch + 2*i] = magOut[i];

#ifdef DEBUG_AUDIO_STREAM_COLUMN_READER_PROCESS
                cerr << "columns[" << colSize * ch + 2*i + 1 << "] <- phaseOut[" << i << "] = " << phaseOut[i] << endl;
#endif

                columns[colSize * ch + 2*i + 1] = phaseOut[i];
            }

            if (magSum) {
                v_add(magSum, magOut, hs1);
            }
        }

        if (columnNo == curvesLength()) {
            // append a metadata calculation as well, using the sum of
            // the magnitudes as the inputs (rather than a single
            // magnitude calculated from a mixture of the time-domain
            // channels, which we don't have available here) -- this
            // is the same as Rubber Band does in RT mode
            turbot_sample_t *m = magSum;
            if (!m) m = magOut;
            addMagMetadata(m);
        }
        
        m_columnCache->update(colSize * m_channels * columnNo,
                              columns,
                              colSize * m_channels);

        deallocate(in);
        deallocate(magOut);
        deallocate(phaseOut);
        if (magSum) deallocate(magSum);
    }

    void processColumnFromStreamCacheMetadataOnly() {

        int columnNo = m_streamCacheColumnNo;

        if (columnNo > curvesLength()) {
            cerr << "processColumnFromStreamCacheMetadataOnly: column " << columnNo << " found, but audio curve only reaches " << int(curvesLength())-1 << " -- can't append (no way to fill the gap)" << endl;
            throw PreconditionFailed("processColumnFromStreamCacheMetadataOnly: column to right of audio curve (would leave a gap)");
        }
        if (columnNo < curvesLength()) {
            return; // nothing to do
        }
        
        int sz = m_timebase.getColumnSize();
        int hs1 = sz/2 + 1;
        int colSize = hs1 * 2;

        turbot_sample_t *in = allocate<turbot_sample_t>(sz);
        turbot_sample_t *magOut = allocate<turbot_sample_t>(hs1);
        turbot_sample_t *magSum = allocate<turbot_sample_t>(hs1);

        // Sadly we have to process both channels here so we can
        // calculate a magnitude sum (for consistency with the
        // calculation in processColumnFromStreamCache above). We
        // prioritise efficient working of that function rather than
        // this one, as it's the normal playback case

        v_zero(magSum, hs1);

        for (int ch = 0; ch < m_channels; ++ch) {

            v_convert(in, m_streamCache[ch], sz);

            addUniquePowerAndPeak(in);

            m_window->cut(in);
            v_fftshift(in, sz);

            addTotalPower(in);

            m_fft->forwardMagnitude(in, magOut);

            v_add(magSum, magOut, hs1);
        }

        addMagMetadata(magSum);
        
        deallocate(in);
        deallocate(magOut);
        deallocate(magSum);
    }

    void addUniquePowerAndPeak(const turbot_sample_t *const in) {

        if (!(m_metadataFlags & UniquePower) &&
            !(m_metadataFlags & PeakSample)) {
            m_peaks.push_back(0);
            m_uniquePowers.push_back(0);
            return;
        }

        turbot_sample_t peak = 0.0;
        turbot_sample_t uniquePower = 0.0;
        
        int sz = m_timebase.getColumnSize();
        int hop = m_timebase.getHop();

        int base = sz/2 - hop/2;
        for (int i = 0; i < hop; ++i) {
            turbot_sample_t value = in[base + i];
            if (fabs(value) > peak) peak = fabs(value);
            uniquePower += value * value;
        }
        
        m_peaks.push_back(peak);
        m_uniquePowers.push_back(uniquePower);
    }

    void addTotalPower(const turbot_sample_t *const in) {

        if (!(m_metadataFlags & TotalPower)) {
            m_totalPowers.push_back(0);
            return;
        }

        turbot_sample_t totalPower = 0.0;
        
        int sz = m_timebase.getColumnSize();

        for (int i = 0; i < sz; ++i) {
            totalPower += in[i] * in[i];
        }
        
        m_totalPowers.push_back(totalPower);
    }

    void addMagMetadata(const turbot_sample_t *const mags) {

        if (m_metadataFlags & AudioCurve) {
            turbot_sample_t val = m_audioCurve->process(mags, m_timebase.getHop());
            m_df.push_back(val);
        } else {
            m_df.push_back(0);
        }

        if (m_metadataFlags & Pitch) {
            turbot_sample_t val = m_pitchCurve->process(mags, m_timebase.getHop());
#ifdef DEBUG_AUDIO_STREAM_COLUMN_READER
            std::cerr << "AudioStreamColumnReader: calculating pitch: val = " << val << std::endl;
#endif
            m_pitch.push_back(val);
            turbot_sample_t confidence = m_pitchCurve->getConfidence();
            m_pitchConfidence.push_back(confidence);
        } else {
            m_pitch.push_back(0);
            m_pitchConfidence.push_back(0);
        }
    }

    QString m_filename;

    AudioReadStream *m_stream;

    int m_channels;
    Timebase m_timebase;

    Window<turbot_sample_t> *m_window;
    FFT *m_fft;

    DataValueCache<turbot_sample_t> *m_columnCache; // columns interleaved

    int curvesLength() { return (int)m_df.size(); }

    int m_metadataFlags;

    vector<float> m_totalPowers;
    vector<float> m_uniquePowers;
    vector<float> m_peaks;

    AudioCurveCalculator *m_audioCurve;
    vector<float> m_df;

    AudioCurveCalculator *m_pitchCurve;
    vector<float> m_pitch;
    vector<float> m_pitchConfidence;
    
    float **m_streamCache; // per channel
    int m_streamCacheColumnNo;

    MusicDatabase::ID m_id;

    int m_retrievalRate;
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
AudioStreamColumnReader::setRetrievalSampleRate(int rate)
{
    m_d->setRetrievalSampleRate(rate);
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

int
AudioStreamColumnReader::getSupportedMetadata() const
{
    return m_d->getSupportedMetadata();
}

int
AudioStreamColumnReader::getDefaultMetadata() const
{
    return m_d->getDefaultMetadata();
}

void
AudioStreamColumnReader::setRequiredMetadata(int mflags)
{
    m_d->setRequiredMetadata(mflags);
}

bool
AudioStreamColumnReader::getPhaseSync(int x)
{
    return m_d->getPhaseSync(x);
}

bool
AudioStreamColumnReader::getHumanOnset(int x)
{
    return m_d->getHumanOnset(x);
}

turbot_sample_t
AudioStreamColumnReader::getAudioCurveValue(int x)
{
    return m_d->getAudioCurveValue(x);
}

turbot_sample_t
AudioStreamColumnReader::getColumnUniquePower(int x, int channel)
{
    return m_d->getColumnUniquePower(x, channel);
}

turbot_sample_t
AudioStreamColumnReader::getColumnTotalPower(int x, int channel)
{
    return m_d->getColumnTotalPower(x, channel);
}

turbot_sample_t
AudioStreamColumnReader::getPitchValue(int x, turbot_sample_t &confidence)
{
    return m_d->getPitchValue(x, confidence);
}

void
AudioStreamColumnReader::close()
{
    m_d->close();
}

}

