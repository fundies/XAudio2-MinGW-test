# Build the XAudio2+Vorbis example

CC= i486-mingw32-g++
LDFLAGS += -static-libstdc++ -static-libgcc -static
CFLAGS= -g -Wall -IInclude/ -IInclude/vorbis

LIBS=-lole32 -lvorbisfile -lvorbis -logg

default: play.exe

play.exe: play.o audio.o
	$(CC) $(LDFLAGS) -o $@ $^  $(LIBS)

%.o: %.cpp
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ -c $<

clean:
	rm *.o play.exe
