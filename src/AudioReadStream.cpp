/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioReadStream.h"
#include "system/Debug.h"

namespace Turbot
{
	
AudioReadStream::FileDRMProtected::FileDRMProtected(QString file) throw() :
    m_file(file)
{
    std::cerr << "ERROR: File is DRM protected: " << file << std::endl;
}

const char *
AudioReadStream::FileDRMProtected::what() const throw()
{
    return QString("File \"%1\" is protected by DRM")
        .arg(m_file).toLocal8Bit().data();
}

}

