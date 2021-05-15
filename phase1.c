// For testing logging/parsing DNS requests

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef uint8_t byte;

struct header {
  uint16_t id;
  uint16_t flags;
  uint16_t qd_count; // assume to be 1
  uint16_t an_count;
  uint16_t ns_count;
  uint16_t ar_count;
};

struct question {
  byte *name;
  uint16_t type;  // assume to be AAAA records (value of 28)
  uint16_t class; // assume to IN (value of 1)
};

struct resource {
  byte *name;
  uint16_t type;
  uint16_t class;
  uint32_t ttl;
  uint16_t rd_length;
  byte *rdata;
};

struct packet {
  struct header header;
  struct question question;
  struct resource *answers;
  struct resource *authority;
  struct resource *additional;
};

int main(int argc, char *argv[]) {
  uint16_t sz; // stores the length of the packet in network order
  if (read(STDIN_FILENO, &sz, 2) < 0) {
    perror("reading size");
    exit(EXIT_FAILURE);
  }
  sz = ntohs(sz); // convert to host order

  byte *buffer = (byte *)malloc(sz);
  assert(buffer);

  if (read(STDIN_FILENO, buffer, sz) < 0) {
    perror("reading buffer");
    exit(EXIT_FAILURE);
  }

  return 0;
}
