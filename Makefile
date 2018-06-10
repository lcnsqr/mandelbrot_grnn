CC=gcc
OPTIONS=-O2

all: gerador grnn

gerador: gerador.c
#	$(CC) gerador.c -lm -lpthread `sdl2-config --cflags --libs` $(OPTIONS) -o $@
	$(CC) gerador.c -lm -lpthread $(OPTIONS) -o $@

grnn: grnn.c
	$(CC) grnn.c -lm $(OPTIONS) -o $@

clean:
	rm gerador grnn
