SDL_CFLAGS=$(shell sdl-config --cflags)
SDL_LIBS=$(shell sdl-config --libs)
CFLAGS=-Wall -O2 $(SDL_CFLAGS)

all:	gauntlet

clean:
	-rm gauntlet
	-rm *.o

gauntlet:	gauntlet.o
	$(CC) -o gauntlet gauntlet.o $(SDL_LIBS)

gauntlet.o:	gauntlet.c

