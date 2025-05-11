#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void panic(const char *s) {
  fprintf(stderr, "panic: ");
  perror(s);
  exit(-1);
}

void produce(int fd) {
  uint64_t value = 0;

  while (true) {
    int r = pwrite(fd, &value, sizeof(value), 0);
    if (r == -1) panic("failed to write to file");
    value++;
  }
}

void consume(int fd) {
  uint64_t value = 0;
  uint64_t prev = 0;

  uint64_t i = 0;

  while (true) {
    int r = pread(fd, &value, sizeof(value), 0);
    if (r == -1) panic("failed to write to file");

    if (value < prev) {
      fprintf(stderr, "value %llu < prev %llu\n", value, prev);
      exit(-1);
    }

    if (value != prev) i++;

    if (i > 1000000) {
      printf("value %llu\n", value);
      i = 0;
    }

    prev = value;
  }
}

int main(int argc, char *argv[]) {
  uint64_t value = 0;

  int fd = open("smoke.dat", O_CREAT | O_RDWR, 0666);
  if (fd == -1) panic("failed to create file");

  int r = ftruncate(fd, sizeof(value));
  if (r == -1) panic("failed to truncate file");

  if (argc > 1) {
    if (strcmp(argv[1], "prod") == 0) {
      produce(fd);
    } else if (strcmp(argv[1], "cons") == 0) {
      consume(fd);
    } else {
      fprintf(stderr, "unknown mode %s\n", argv[1]);
      return -1;
    }
  } else {
    fprintf(stderr, "usage: %s <prod|cons>\n", argv[0]);
    return -1;
  }
}
