/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioDirectoryIndex.h"

#include <QMutexLocker>
#include <QMutex>
#include <QThread>
#include <QDir>
#include <QFileInfo>

#include "system/Debug.h"
#include "AudioReadStreamFactory.h"
#include "AudioReadStream.h"

namespace Turbot {

class AudioDirectoryIndex::D : public QThread
{
public:
    D(AudioDirectoryIndex *adi, QString dirPath) :
	m_adi(adi), m_dirPath(dirPath), m_complete(false),
	m_total(0), m_progress(0), m_percentage(0), m_cancelled(false) {
    }

    ~D() {
    }

    void cancel() {
	m_cancelled = true; 
    }

    int getGoodFileCount() const {
	QMutexLocker locker(&m_mutex);
	return m_good.size();
    }
    int getUnsupportedFileCount() const {
	QMutexLocker locker(&m_mutex);
	return m_unsupported.size();
    }
    int getProtectedFileCount() const {
	QMutexLocker locker(&m_mutex);
	return m_protected.size();
    }
    
    QStringList getGoodFiles() const {
	QMutexLocker locker(&m_mutex);
	return m_good;
    }
    QStringList getUnsupportedFiles() const {
	QMutexLocker locker(&m_mutex);
	return m_unsupported;
    }
    QStringList getProtectedFiles() const {
	QMutexLocker locker(&m_mutex);
	return m_protected;
    }

    bool isComplete() const {
	QMutexLocker locker(&m_mutex);
	return m_complete;
    }

    int getCompletion() const {
	QMutexLocker locker(&m_mutex);
	return m_percentage;
    }

    void run() {
	QDir dir(m_dirPath);
	if (dir.exists()) {
	    index(dir);
	}
	m_complete = true;
	m_adi->emitComplete();
    }

    void index(QDir dir) {

	QStringList entries = dir.entryList(QDir::Files | QDir::Readable);
	m_total = entries.count();

	foreach (QString entry, entries) {

	    if (m_cancelled) return;

	    QString filePath = dir.absoluteFilePath(entry);
	    QFileInfo fi(filePath);
	    if (!fi.exists() || !fi.isFile() || !fi.isReadable()) {
		DEBUG << "AudioDirectoryIndex::index: Note: entry " << entry
		      << " in dir " << dir
		      << " is nonexistent, not a file, or not readable"
		      << endl;
		continue;
	    }

	    indexEntry(filePath);

	    ++m_progress;

	    if (updatePercentage()) {
		m_adi->emitProgressUpdated(m_percentage);
	    }
	}
    }

    void indexEntry(QString filePath) {
	AudioReadStream *stream = 0;
	bool good = true;
	try {
	    stream = AudioReadStreamFactory::createReadStream(filePath);
	    if (!stream) {
		good = false;
	    } else if (stream->getChannelCount() == 0 ||
		       stream->getSampleRate() == 0) {
		DEBUG << "AudioDirectoryIndex::indexEntry: Note: file "
		      << filePath << " has null channel count or sample rate"
		      << endl;
		good = false;
	    }
	} catch (AudioReadStream::FileDRMProtected &) {
	    QMutexLocker locker(&m_mutex);
	    m_protected.push_back(filePath);
	    delete stream;
	    return;
	} catch (...) {
	    good = false;
	}

	delete stream;

	QMutexLocker locker(&m_mutex);
	if (good) {
	    m_good.push_back(filePath);
	} else {
	    m_unsupported.push_back(filePath);
	}
    }

private:
    AudioDirectoryIndex *m_adi;

    QString m_dirPath;
    QStringList m_good;
    QStringList m_unsupported;
    QStringList m_protected;
    bool m_complete;
    int m_total;
    int m_progress;
    int m_percentage;
    bool m_cancelled;
    mutable QMutex m_mutex;

    bool updatePercentage() {
	if (m_complete) return 100;
	if (m_total == 0) return 0;
	int percentage = (m_progress * 100) / m_total;
	if (percentage != m_percentage) {
	    m_percentage = percentage;
	    return true;
	} else {
	    return false;
	}
    }
};

AudioDirectoryIndex::AudioDirectoryIndex(QString dirPath) :
    m_d(new D(this, dirPath))
{
    m_d->start();
}

AudioDirectoryIndex::~AudioDirectoryIndex()
{
    cancel();
    delete m_d;
}

void
AudioDirectoryIndex::cancel()
{
    m_d->cancel();
    wait();
}

void
AudioDirectoryIndex::wait()
{
    m_d->wait();
}

bool
AudioDirectoryIndex::isComplete() const
{
    return m_d->isComplete();
}

int
AudioDirectoryIndex::getCompletion() const
{
    return m_d->getCompletion();
}

int
AudioDirectoryIndex::getGoodFileCount() const
{
    return m_d->getGoodFileCount();
}

int
AudioDirectoryIndex::getUnsupportedFileCount() const
{
    return m_d->getUnsupportedFileCount();
}

int
AudioDirectoryIndex::getProtectedFileCount() const
{
    return m_d->getProtectedFileCount();
}

QStringList
AudioDirectoryIndex::getGoodFiles() const
{
    return m_d->getGoodFiles();
}

QStringList
AudioDirectoryIndex::getUnsupportedFiles() const
{
    return m_d->getUnsupportedFiles();
}

QStringList
AudioDirectoryIndex::getProtectedFiles() const
{
    return m_d->getProtectedFiles();
}

void
AudioDirectoryIndex::emitComplete()
{
    emit complete();
}

void
AudioDirectoryIndex::emitProgressUpdated(int p)
{
    emit progressUpdated(p);
}

}

