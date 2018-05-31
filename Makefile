CC=gcc
OPTIONS=-O3

all: gerador

gerador: gerador.c
	$(CC) gerador.c -lm -lpthread $(OPTIONS) -o $@

clean:
	rm gerador
