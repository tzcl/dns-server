#include "packet.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define QNAME_LEN 256 // max length of qname is 255 octets

/**
 * Reads in the next message from the given file descriptor.
 * Caller is responsible for freeing the resulting struct */
struct packet *parse_packet(int fd) {
  uint16_t sz;
  if (read(STDIN_FILENO, &sz, 2) < 0) {
    perror("reading size");
    exit(EXIT_FAILURE);
  }
  sz = ntohs(sz);

  byte *buffer = (byte *)malloc(sz);
  assert(buffer);

  size_t n;
  while ((n = read(STDIN_FILENO, buffer, sz)) < sz) {
    if (n < 0) {
      perror("reading buffer");
      exit(EXIT_FAILURE);
    }
  }

  struct packet *packet = (struct packet *)malloc(sizeof packet);
  assert(packet);

  byte *buffer_ptr = buffer;
  parse_header(packet, &buffer_ptr);
  if (is_response(packet)) {
    printf("Is response!\n");
  } else {
    printf("Is query!\n");
  }

  free(buffer);

  return packet;
}

/**
 * Populates the packet header and increments the buffer pointer */
void parse_header(struct packet *packet, byte **buffer) {
  size_t inc = sizeof(uint16_t) / sizeof(byte);

  byte *buffer_ptr = *buffer;

  packet->header.id = ntohs(*((uint16_t *)buffer_ptr)), buffer_ptr += inc;
  packet->header.flags = ntohs(*((uint16_t *)buffer_ptr)), buffer_ptr += inc;
  packet->header.qd_count = ntohs(*((uint16_t *)buffer_ptr)), buffer_ptr += inc;
  packet->header.an_count = ntohs(*((uint16_t *)buffer_ptr)), buffer_ptr += inc;
  packet->header.ns_count = ntohs(*((uint16_t *)buffer_ptr)), buffer_ptr += inc;
  packet->header.ar_count = ntohs(*((uint16_t *)buffer_ptr)), buffer_ptr += inc;
}

/**
 * Populates the packet question and increments the buffer pointer */
void parse_question(struct packet *packet, byte **buffer) {
  char qname_buffer[QNAME_LEN];
  byte *buffer_ptr = *buffer;

  byte label_len = *buffer_ptr++;
  size_t index = 0;
  while (label_len) {
    label_len += index;
    while (index < label_len) {
      qname_buffer[index++] =
          (char)*buffer_ptr++; // should we be casting to char?
    }
    qname_buffer[index++] = '.';
    label_len = *buffer_ptr++;
  }
  qname_buffer[--index] = '\0'; // remove the final '.'

  packet->question.name = (char *)malloc(strlen(qname_buffer));
  assert(packet->question.name);
  strcpy(packet->question.name, qname_buffer);

  size_t inc = sizeof(uint16_t) / sizeof(byte);
  packet->question.type = ntohs(*((uint16_t *)buffer)), buffer += inc;
  packet->question.qclass = ntohs(*((uint16_t *)buffer)), buffer += inc;

  // TODO: need to check that the question type is AAAA
}

/**
 * Populates the packet answer and increments the buffer pointer */
void parse_answer(struct packet *packet, byte **buffer) {}

/**
 * Returns 1 if the packet is a response, else returns 0  */
int is_response(struct packet *packet) { return packet->header.flags >> 15; }
