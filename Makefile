
SOURCES	:= src/AudioReadStream.cpp src/AudioReadStreamFactory.cpp src/AudioWriteStreamFactory.cpp src/Exceptions.cpp
HEADERS	:= $(wildcard src/*.h)
OBJECTS	:= $(patsubst %.cpp,%.o,$(SOURCES))
LIBRARY	:= libbqaudiostream.a

CXXFLAGS := -DHAVE_LIBSNDFILE -I../bqvec -I../bqthingfactory -I../bqresample -I./bqaudiostream -fpic

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
src/AudioReadStreamFactory.o: src/MediaFoundationReadStream.cpp
src/AudioReadStreamFactory.o: src/CoreAudioReadStream.cpp
src/AudioReadStreamFactory.o: src/BasicMp3ReadStream.cpp
src/AudioWriteStreamFactory.o: ./bqaudiostream/AudioWriteStreamFactory.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/AudioWriteStream.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/Exceptions.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/AudioReadStreamFactory.h
src/AudioWriteStreamFactory.o: src/WavFileWriteStream.cpp
src/AudioWriteStreamFactory.o: src/SimpleWavFileWriteStream.cpp
src/AudioWriteStreamFactory.o: src/SimpleWavFileWriteStream.h
src/AudioWriteStreamFactory.o: src/CoreAudioWriteStream.cpp
src/AudioWriteStreamFactory.o: src/CoreAudioWriteStream.h
src/MediaFoundationReadStream.o: ./bqaudiostream/AudioReadStream.h
src/WavFileWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/SimpleWavFileWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/WavFileReadStream.o: ./bqaudiostream/AudioReadStream.h
src/CoreAudioWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/CoreAudioReadStream.o: ./bqaudiostream/AudioReadStream.h
src/BasicMp3ReadStream.o: ./bqaudiostream/AudioReadStream.h
src/OggVorbisReadStream.o: ./bqaudiostream/AudioReadStream.h
