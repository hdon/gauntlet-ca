# Makefile for Gauntlet
#
# Bill Kendrick <bill@newbreedsoftware.com>
# http://www.newbreedsoftware.com/
#
# February 25, 2009 - February 26, 2009

SDL_CFLAGS=$(shell sdl-config --cflags)
SDL_LIBS=$(shell sdl-config --libs) -lSDL_image
CFLAGS=-Wall -O2 $(SDL_CFLAGS) -ggdb3

all:	gauntlet

clean:
	rm -f *.o gauntlet

gauntlet: gauntlet.o noise.o
	$(CC) -o $@ $^ $(SDL_LIBS)


