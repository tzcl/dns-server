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

  size_t n;
  while ((n = read(STDIN_FILENO, buffer, sz)) < sz) {
    // TODO: fix up this loop
    printf("%zu\n", n);
    if (n < 0) {
      perror("reading buffer");
      exit(EXIT_FAILURE);
    }
  }

  // PARSE DNS MESSAGE
  struct packet packet;

  // populate header
  size_t inc = sizeof(uint16_t) / sizeof(byte);
  packet.header.id = ntohs(*((uint16_t *)buffer)), buffer += inc;
  packet.header.flags = ntohs(*((uint16_t *)buffer)), buffer += inc;
  packet.header.qd_count = ntohs(*((uint16_t *)buffer)), buffer += inc;
  packet.header.an_count = ntohs(*((uint16_t *)buffer)), buffer += inc;
  packet.header.ns_count = ntohs(*((uint16_t *)buffer)), buffer += inc;
  packet.header.ar_count = ntohs(*((uint16_t *)buffer)), buffer += inc;

  // TODO: switch depending on QR bit
  // should print out all the flags

  // populate question
  byte qname_buffer[256]; // max len of qname is 255 octets
  byte label_len = *buffer++;
  size_t index = 0;
  while (label_len) {
    label_len += index;
    while (index < label_len) {
      qname_buffer[index++] = *buffer++; // should we be casting to char?
    }
    qname_buffer[index++] = '.';
    label_len = *buffer++;
  }
  qname_buffer[--index] = 0; // remove the final '.'

  packet.question.name = qname_buffer; // local scope!

  packet.question.type = ntohs(*((uint16_t *)buffer)), buffer += inc;
  packet.question.class = ntohs(*((uint16_t *)buffer)), buffer += inc;

  // can append the rest of the buffer?
  // or do I need to parse the AR record?

  // LOGGING
  FILE *log = fopen("dns_svr.log", "w");
  if (!log) {
    perror("opening log");
    exit(EXIT_FAILURE);
  }

  time_t rawtime = time(NULL);
  struct tm *timeinfo = gmtime(&rawtime);

  char time_buf[256];
  strftime(time_buf, 256, "%FT%T%z", timeinfo);
  fprintf(log, "%s requested %s\n", time_buf, packet.question.name);
  fflush(log);

  fclose(log);

  return 0;
}
