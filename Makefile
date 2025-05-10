CC=gcc
CFLAGS=-std=c2x -O3 -Wall -Werror -g

all: spscq

spqcq: spscq.c
	$(CC) $(CFLAGS) $< -o $@

format:
	clang-format -i spscq.c

clean:
	-rm spscq

.PHONY: all clean format
