
SOURCES	:= $(wildcard src/*.cpp)
HEADERS	:= $(wildcard src/*.h)
OBJECTS	:= $(patsubst %.cpp,%.o,$(SOURCES))
LIBRARY	:= libbqaudiostream.a

CXXFLAGS := -I../bqvec -I../bqthingfactory -I../bqresample -I./bqaudiostream

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	ar cr $@ $<


