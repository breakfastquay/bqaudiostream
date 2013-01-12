
#include "AudioStreamColumnReader.h"
#include "AudioReadStreamFactory.h"

// Include this into your main file to defined the factories in question

namespace Turbot {

RegionColumnReaderBuilder<AudioStreamColumnReader>
AudioStreamColumnReader::m_colBuilder(
    "http://breakfastquay.com/rdf/turbot/regionreader/AudioStreamColumnReader",
    AudioReadStreamFactory::getSupportedFileExtensions()
);

RegionMetadataReaderBuilder<AudioStreamColumnReader>
AudioStreamColumnReader::m_metaBuilder(
    "http://breakfastquay.com/rdf/turbot/regionreader/AudioStreamColumnReader",
    AudioReadStreamFactory::getSupportedFileExtensions()
);

}

