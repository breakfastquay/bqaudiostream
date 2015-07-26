
SOURCES	:= $(wildcard src/*.cpp)
HEADERS	:= $(wildcard src/*.h)
OBJECTS	:= $(patsubst %.cpp,%.o,$(SOURCES))
LIBRARY	:= libbqaudiostream.a

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	ar cr $@ $<


