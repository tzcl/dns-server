// For testing logging/parsing DNS requests

#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef uint8_t byte;

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
