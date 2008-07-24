all: bcastd

clean:
	[ ! -e bcastd ] || ( rm bcastd )
	[ ! -e bcastd.core ] || ( rm bcastd.core )

CFLAGS=-ansi -pedantic -O3 -g -Wall
