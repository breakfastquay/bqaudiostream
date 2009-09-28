/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/* Copyright Chris Cannam - All Rights Reserved */

#include "AudioFileReadStream.h"

#include <iostream>
#include <sstream>

using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;

namespace Turbot
{
	
AudioFileReadStream::FileDRMProtected::FileDRMProtected(string file) throw() :
    m_file(file)
{
    cerr << "ERROR: File is DRM protected: " << file << endl;
}

const char *
AudioFileReadStream::FileDRMProtected::what() const throw()
{
    ostringstream os;
    os << "File \"" << m_file << "\" is protected by DRM";
    return os.str().c_str();
}

}

