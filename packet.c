#include "packet.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define QNAME_LEN 256
#define AAAA_RECORD 28

struct packet *init_packet() {
  struct packet *packet = (struct packet *)malloc(sizeof *packet);
  assert(packet);

  packet->question.name = NULL;
  packet->answer.name = NULL;
  packet->answer.rdata = NULL;
  packet->answer.address = NULL;
  packet->remaining = NULL;

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
  if (packet->answer.name) {
    free(packet->answer.name);
  }
  if (packet->answer.rdata) {
    free(packet->answer.rdata);
  }

  // Clean up remaining byte data
  if (packet->remaining) {
    free(packet->remaining);
  }

  free(packet);
}

/**
 * Reads in the next message from the given file descriptor.
 * Caller is responsible for freeing the resulting struct.
 */
struct packet *parse_packet(byte *buffer) {
  struct packet *packet = init_packet();

  byte *buffer_ptr = buffer;
  parse_header(packet, &buffer_ptr);

  if (packet->header.qd_count)
    parse_question(packet, &buffer_ptr);

  if (packet->header.an_count)
    parse_answer(packet, &buffer_ptr);

  return packet;
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

  packet->question.name = (char *)malloc(index);
  assert(packet->question.name);
  memcpy(packet->question.name, qname_buffer, index);

  size_t inc = sizeof(uint16_t) / sizeof(byte);
  packet->question.type = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
  packet->question.qclass = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
}

/**
 * Populates the packet answer and increments the buffer pointer */
void parse_answer(struct packet *packet, byte **buffer) {
  size_t inc = sizeof(uint16_t) / sizeof(byte);

  // TODO: dirty hack because I don't want to do this properly
  packet->answer.name = (char *)malloc(strlen(packet->question.name) + 1);
  assert(packet->answer.name);
  strcpy(packet->answer.name, packet->question.name);
  *buffer += inc;

  packet->answer.type = packet->question.type, *buffer += inc;
  packet->answer.rclass = packet->question.qclass, *buffer += inc;

  packet->answer.ttl = ntohl(*((uint32_t *)*buffer)), *buffer += 2 * inc;

  packet->answer.rd_length = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
  packet->answer.rdata = (byte *)malloc(packet->answer.rd_length);
  assert(packet->answer.rdata);
  for (int i = 0; i < packet->answer.rd_length; i++) {
    packet->answer.rdata[i] = *(*buffer)++;
  }

  packet->answer.address = (char *)malloc(INET6_ADDRSTRLEN);
  assert(packet->answer.address);
  inet_ntop(AF_INET6, packet->answer.rdata, packet->answer.address,
            INET6_ADDRSTRLEN);
}

/**
 * Returns 1 if the packet is a response, else returns 0  */
int is_response(struct packet *packet) { return packet->header.flags >> 15; }

/**
 * Returns 1 if the request is for an AAAA record, else returns 0
 */
int is_AAAA_request(struct packet *packet) {
  return packet->question.type == AAAA_RECORD;
}

/**
 * Returns 1 if the first answer is an AAAA field, else returns 0
 */
int is_AAAA_response(struct packet *packet) {
  return packet->answer.type == AAAA_RECORD;
}

/**
 * Sets the rcode in the header to 4 (unimplemented request) */
void set_unimpl_rcode(struct packet *packet) {
  packet->header.flags |= 1 << 2;
  packet->header.flags |= 1 << 15;

  packet->header.qd_count = 0;
  packet->header.an_count = 0;
  packet->header.ns_count = 0;
  packet->header.ar_count = 0;
}
