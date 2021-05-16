#include "packet.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define QNAME_LEN 256

struct packet *init_packet() {
  struct packet *packet = (struct packet *)malloc(sizeof(*packet));
  assert(packet);

  packet->question.name = NULL;
  packet->answer.name = NULL;
  packet->answer.rdata = NULL;
  packet->remaining = NULL;

  return packet;
}

/**
 * Reads in the next message from the given file descriptor and writes messages
 * to the provided log. Caller is responsible for freeing the resulting struct.
 */
struct packet *parse_packet(int fd, FILE *log) {
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

  struct packet *packet = init_packet();

  byte *buffer_ptr = buffer;
  parse_header(packet, &buffer_ptr);
  if (is_response(packet)) {
    printf("Is response!\n");
  } else {
    parse_question(packet, &buffer_ptr);
    write_request(log, packet->question.name);
  }

  free(buffer);

  // TODO: append remaining bytes

  return packet;
}

/**
 * Frees the memory allocated to a packet */
void free_packet(struct packet *packet) {
  // Clean up question
  if (packet->question.name) {
    free(packet->question.name);
  }

  // Clean up answer
  // TODO: implement

  // Clean up remaining byte data
  if (packet->remaining) {
    free(packet->remaining);
  }

  free(packet);
}

/**
 * Populates the packet header and increments the buffer pointer */
void parse_header(struct packet *packet, byte **buffer) {
  size_t inc = sizeof(uint16_t) / sizeof(byte);

  packet->header.id = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
  packet->header.flags = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
  packet->header.qd_count = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
  packet->header.an_count = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
  packet->header.ns_count = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
  packet->header.ar_count = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
}

/**
 * Populates the packet question and increments the buffer pointer */
void parse_question(struct packet *packet, byte **buffer) {
  char qname_buffer[QNAME_LEN];

  byte label_len = *(*buffer)++;
  size_t index = 0;
  while (label_len) {
    label_len += index;
    while (index < label_len) {
      qname_buffer[index++] = *(*buffer)++;
    }
    qname_buffer[index++] = '.';
    label_len = *(*buffer)++;
  }
  qname_buffer[index - 1] = '\0'; // remove the final '.'

  packet->question.name = (char *)malloc(index * sizeof(char));
  assert(packet->question.name);
  memcpy(packet->question.name, qname_buffer, index);

  size_t inc = sizeof(uint16_t) / sizeof(byte);
  packet->question.type = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
  packet->question.qclass = ntohs(*((uint16_t *)*buffer)), *buffer += inc;

  // TODO: need to check that the question type is AAAA
}

/**
 * Populates the packet answer and increments the buffer pointer */
void parse_answer(struct packet *packet, byte **buffer) {}

/**
 * Returns 1 if the packet is a response, else returns 0  */
int is_response(struct packet *packet) { return packet->header.flags >> 15; }
