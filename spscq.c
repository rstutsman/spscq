#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/time.h>
#include <unistd.h>

const bool DEBUG = true;

const int head_offset = 0;
const int tail_offset = head_offset + sizeof(int);
const int elts_per_page = 4096 / sizeof(int);

typedef enum { PROD, CONS, PROD_CONS } spsc_mode_t;

spsc_mode_t mode;
int n_ops = 1000000;

int locked = false;
int metafd = -1;
int datafd = -1;

void panic(const char *s) {
  fprintf(stderr, "panic: ");
  perror(s);
  exit(-1);
}

void debug(const char *s) {
  if (!DEBUG) return;
  fprintf(stderr, "debug: %s\n", s);
}

// copied from fxmark
uint64_t usec(void) {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

void _lock(int fd, int op) {
  if (!locked) return;
  int r = flock(fd, op);
  if (r == -1) panic("_lock failed");
}

void lock(int fd) { _lock(fd, LOCK_EX); }

void unlock(int fd) { _lock(fd, LOCK_UN); }

typedef enum { PREAD, PWRITE } op_t;

void io(op_t op, int fd, off_t offset, int *value) {
  lock(fd);
  int r;
  do {
    if (op == PREAD)
      r = pread(fd, value, sizeof(*value), offset);
    else
      r = pwrite(fd, value, sizeof(*value), offset);
    if (r == -1) panic("i/o failed");
  } while (r != sizeof(*value));
  unlock(fd);
}

int load_tail() {
  int value;
  io(PREAD, metafd, tail_offset, &value);
  return value;
}

void store_tail(int value) { io(PWRITE, metafd, tail_offset, &value); }

int load_head() {
  int value;
  io(PREAD, metafd, head_offset, &value);
  return value;
}

void store_head(int value) { io(PWRITE, metafd, head_offset, &value); }

int consume(int index) {
  int value;
  off_t off = (index % elts_per_page) * sizeof(int);
  io(PREAD, datafd, off, &value);
  return value;
}

void produce(int index, int value) {
  off_t off = (index % elts_per_page) * sizeof(int);
  io(PWRITE, datafd, off, &value);
}

void consumer() {
  uint64_t start_time = usec();

  int prev = -10;
  for (int i = 0; i < n_ops;) {
    int head;
    do {
      head = load_head();
    } while (head <= i);

    int value = consume(i);

    assert(value == prev + 10);
    prev = value;

    i++;

    store_tail(i);
  }

  uint64_t end_time = usec();

  uint64_t us = end_time - start_time;
  printf("elapsed time %f s\n", us / 1e6);
  printf("tput %f op/s\n", n_ops / (us / 1e6));
}

void producer() {
  int value = 0;
  for (int i = 0; i < n_ops;) {
    int tail;
    do {
      tail = load_tail();
    } while (tail + elts_per_page <= i);

    produce(i, value);

    i++;
    value += 10;

    store_head(i);
  }

  printf("producer done\n");
}

int main(int argc, char *argv[]) {
  if (argc < 5) {
    fprintf(stderr,
            "usage: %s <prod,cons,prod-cons> <n_ops> <data_file_path> "
            "<metadata_file_path> [-l]\n",
            argv[0]);
    return -1;
  }

  if (strcmp(argv[1], "prod") == 0)
    mode = PROD;
  else if (strcmp(argv[1], "cons") == 0)
    mode = CONS;
  else if (strcmp(argv[1], "prod-cons") == 0)
    mode = PROD_CONS;
  else {
    fprintf(stderr, "unknown mode %s\n", argv[1]);
    return -1;
  }

  n_ops = atoi(argv[2]);

  const char *data_path = argv[3];
  const char *metadata_path = argv[4];

  if (argc > 5) {
    if (strcmp(argv[5], "-l") == 0) locked = true;
  }

  printf("Mode: %s\n", mode == PROD   ? "PROD"
                       : mode == CONS ? "CONS"
                                      : "PROD_CONS");
  printf("n_ops: %d\n", n_ops);
  printf("data_path: %s\n", data_path);
  printf("metadata_path: %s\n", metadata_path);
  printf("locked: %s\n", locked ? "true" : "false");
  printf("\n");

  datafd = open(data_path, O_CREAT | O_RDWR, 0666);
  if (datafd == -1) panic("failed to create file");

  metafd = open(metadata_path, O_CREAT | O_RDWR, 0666);
  if (metafd == -1) panic("failed to create file");

  // Initialize the metadata file
  // Initialize the metadata file
  int zero = 0;
  io(PWRITE, metafd, head_offset, &zero);
  io(PWRITE, metafd, tail_offset, &zero);

  pid_t pid;
  switch (mode) {
    case PROD:
      producer();
      break;

    case CONS:
      consumer();
      break;

    case PROD_CONS:
      pid = fork();
      if (pid == 0) {
        producer();
        wait(NULL);
      } else {
        consumer();
      }
      break;

    default:
      panic("unknown mode");
  }
}
