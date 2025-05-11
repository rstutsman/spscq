CC=gcc
CFLAGS=-std=c2x -O3 -Wall -Werror -g

all: spscq smoke

spqcq: spscq.c
	$(CC) $(CFLAGS) $< -o $@

smoke: smoke.c
	$(CC) $(CFLAGS) $< -o $@

format:
	clang-format -i spscq.c smoke.c

clean:
	-rm spscq

.PHONY: all clean format
