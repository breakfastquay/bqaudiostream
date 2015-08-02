
SOURCES	:= $(wildcard src/*.cpp)
HEADERS	:= $(wildcard src/*.h)
OBJECTS	:= $(patsubst %.cpp,%.o,$(SOURCES))
LIBRARY	:= libbqaudiostream.a

CXXFLAGS := -I../bqvec -I../bqthingfactory -I../bqresample -I./bqaudiostream

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	ar cr $@ $<

clean:		
	rm -f $(OBJECTS)

distclean:	clean
	rm -f $(LIBRARY)

depend:
	makedepend -Y -fMakefile -I./bqaudiostream $(SOURCES) $(HEADERS)


# DO NOT DELETE

src/AudioReadStreamFactory.o: ./bqaudiostream/AudioReadStreamFactory.h
src/AudioReadStreamFactory.o: src/AudioReadStream.h
src/AudioReadStreamFactory.o: ./bqaudiostream/Exceptions.h
src/AudioReadStreamFactory.o: src/WavFileReadStream.cpp
src/AudioReadStreamFactory.o: src/OggVorbisReadStream.cpp
src/AudioReadStreamFactory.o: src/MediaFoundationReadStream.cpp
src/AudioReadStreamFactory.o: src/CoreAudioReadStream.cpp
src/AudioReadStreamFactory.o: src/BasicMp3ReadStream.cpp
src/SimpleWavFileWriteStream.o: src/SimpleWavFileWriteStream.h
src/SimpleWavFileWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/SimpleWavFileWriteStream.o: ./bqaudiostream/Exceptions.h
src/Exceptions.o: ./bqaudiostream/Exceptions.h
src/AudioReadStream.o: src/AudioReadStream.h
src/CoreAudioWriteStream.o: src/CoreAudioWriteStream.h
src/CoreAudioWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/AudioWriteStreamFactory.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/AudioWriteStream.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/Exceptions.h
src/AudioWriteStreamFactory.o: ./bqaudiostream/AudioReadStreamFactory.h
src/AudioWriteStreamFactory.o: src/WavFileWriteStream.cpp
src/AudioWriteStreamFactory.o: src/SimpleWavFileWriteStream.cpp
src/AudioWriteStreamFactory.o: src/SimpleWavFileWriteStream.h
src/AudioWriteStreamFactory.o: src/CoreAudioWriteStream.cpp
src/AudioWriteStreamFactory.o: src/CoreAudioWriteStream.h
src/MediaFoundationReadStream.o: src/AudioReadStream.h
src/WavFileWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/SimpleWavFileWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/WavFileReadStream.o: src/AudioReadStream.h
src/CoreAudioWriteStream.o: ./bqaudiostream/AudioWriteStream.h
src/CoreAudioReadStream.o: src/AudioReadStream.h
src/BasicMp3ReadStream.o: src/AudioReadStream.h
src/OggVorbisReadStream.o: src/AudioReadStream.h
