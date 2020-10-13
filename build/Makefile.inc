
SOURCES	:= src/AudioReadStream.cpp src/AudioReadStreamFactory.cpp src/AudioWriteStreamFactory.cpp src/AudioStreamExceptions.cpp
HEADERS	:= $(wildcard src/*.h) $(wildcard bqaudiostream/*.h)
OBJECTS	:= $(patsubst %.cpp,%.o,$(SOURCES))
LIBRARY	:= libbqaudiostream.a

CXXFLAGS := -std=c++98 -Wall $(AUDIOSTREAM_DEFINES) -I../bqvec -I../bqthingfactory -I../bqresample -I./bqaudiostream -fpic $(THIRD_PARTY_INCLUDES)

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	ar cr $@ $^

clean:		
	rm -f $(OBJECTS)

distclean:	clean
	rm -f $(LIBRARY)

depend:
	makedepend -Y -fMakefile -I./bqaudiostream $(SOURCES) $(HEADERS)


# DO NOT DELETE

src/AudioReadStream.o: ./bqaudiostream/AudioReadStream.h
src/AudioReadStreamFactory.o: ./bqaudiostream/AudioReadStreamFactory.h
src/AudioReadStreamFactory.o: ./bqaudiostream/AudioReadStream.h
src/AudioReadStreamFactory.o: ./bqaudiostream/Exceptions.h
src/AudioReadStreamFactory.o: src/WavFileReadStream.cpp
src/AudioReadStreamFactory.o: src/OggVorbisReadStream.cpp
src/AudioReadStreamFactory.o: src/MiniMP3ReadStream.cpp
src/AudioReadStreamFactory.o: src/MediaFoundationReadStream.cpp
src/AudioReadStreamFactory.o: src/SimpleWavFileReadStream.cpp
src/AudioReadStreamFactory.o: src/SimpleWavFileReadStream.h
src/AudioReadStreamFactory.o: src/CoreAudioReadStream.cpp
src/AudioReadStreamFactory.o: src/OpusReadStream.cpp
src/AudioWriteStreamFactory.o: ./bqaudiostream/AudioWriteStreamFactory.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/AudioWriteStream.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/Exceptions.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/AudioReadStreamFactory.h
src/AudioWriteStreamFactory.o: src/WavFileWriteStream.cpp
src/AudioWriteStreamFactory.o: src/SimpleWavFileWriteStream.cpp
src/AudioWriteStreamFactory.o: src/SimpleWavFileWriteStream.h
src/AudioWriteStreamFactory.o: src/CoreAudioWriteStream.cpp
src/AudioWriteStreamFactory.o: src/CoreAudioWriteStream.h
src/AudioWriteStreamFactory.o: src/OpusWriteStream.cpp
src/AudioStreamExceptions.o: ./bqaudiostream/Exceptions.h
src/CoreAudioReadStream.o: ./bqaudiostream/AudioReadStream.h
src/CoreAudioWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/MediaFoundationReadStream.o: ./bqaudiostream/AudioReadStream.h
src/MiniMP3ReadStream.o: ./bqaudiostream/AudioReadStream.h
src/OggVorbisReadStream.o: ./bqaudiostream/AudioReadStream.h
src/OpusReadStream.o: ./bqaudiostream/AudioReadStream.h
src/SimpleWavFileReadStream.o: ./bqaudiostream/AudioReadStream.h
src/SimpleWavFileWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/WavFileReadStream.o: ./bqaudiostream/AudioReadStream.h
src/WavFileWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/OpusWriteStream.o: ./bqaudiostream/AudioWriteStream.h
