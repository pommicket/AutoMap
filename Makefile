CC=gcc
CFLAGS=-Wall -lm

default: AutoMap

AutoMap: main.c PPM.h FileIO.h Constants.h
	$(CC) $(CFLAGS) main.c -o AutoMap

run: AutoMap
	./AutoMap

clean:
	rm AutoMap