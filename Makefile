CC = gcc
CFLAGS = -Wall -g

all: bin/ls

bin/ls: src/*.c
	$(CC) $(CFLAGS) src/*.c -o bin/ls

clean:
	rm -f bin/ls obj/*.o

.PHONY: all clean
