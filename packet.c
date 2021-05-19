#include "packet.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define QNAME_LEN 256
#define AAAA_RECORD 28

struct packet *init_packet(uint16_t buf_size) {
  struct packet *packet = (struct packet *)malloc(sizeof *packet);
  assert(packet);

  packet->size = buf_size;

  packet->question.name = NULL;
  packet->answer.name = NULL;
  packet->answer.address = NULL;
  packet->remaining = NULL;

  return packet;
}

/**
 * Frees the memory allocated to a packet */
void free_packet(struct packet *packet) {
  if (packet->question.name) {
    free(packet->question.name);
  }
  if (packet->answer.address) {
    free(packet->answer.address);
  }
  if (packet->remaining) {
    free(packet->remaining);
  }

  free(packet);
}

/**
 * Reads in the next message from the given file descriptor.
 * Caller is responsible for freeing the resulting struct.
 */
struct packet *parse_packet(byte *buffer, uint16_t buf_size) {
  struct packet *packet = init_packet(buf_size);

  byte *buffer_ptr = buffer;
  parse_header(packet, &buffer_ptr);

  if (packet->header.qd_count)
    parse_question(packet, &buffer_ptr);

  if (packet->header.an_count)
    parse_answer(packet, &buffer_ptr);

  uint16_t remaining_size = buf_size - (buffer_ptr - buffer);
  if (remaining_size > 0) {
    packet->remaining = malloc(remaining_size);
    assert(packet->remaining);
    memcpy(packet->remaining, buffer_ptr, remaining_size);
  }

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
  qname_buffer[index - 1] = '\0';

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

  packet->answer.name = packet->question.name;
  packet->answer.original = ntohs(*((uint16_t *)*buffer));
  *buffer += inc;

  packet->answer.type = ntohs(*((uint16_t *)*buffer)), *buffer += inc;
  packet->answer.rclass = ntohs(*((uint16_t *)*buffer)), *buffer += inc;

  packet->answer.ttl = ntohl(*((uint32_t *)*buffer)), *buffer += 2 * inc;

  uint16_t rd_length = ntohs(*((uint16_t *)*buffer));
  *buffer += inc;

  byte rdata[rd_length];
  for (int i = 0; i < rd_length; i++) {
    rdata[i] = *(*buffer)++;
  }

  packet->answer.address = (char *)malloc(INET6_ADDRSTRLEN);
  assert(packet->answer.address);
  inet_ntop(AF_INET6, rdata, packet->answer.address, INET6_ADDRSTRLEN);
}

/**
 * Converts the packet back into a byte stream stored in the given buffer */
byte *buffer_packet(struct packet *packet) {
  byte *buffer = malloc(packet->size);
  assert(buffer);
  byte *buffer_ptr = buffer;

  buffer_header(&packet->header, &buffer_ptr);

  if (packet->header.qd_count)
    buffer_question(&packet->question, &buffer_ptr);
  if (packet->header.an_count)
    buffer_answer(&packet->answer, &buffer_ptr);
  if (packet->remaining)
    memcpy(buffer_ptr, packet->remaining, packet->size - (buffer_ptr - buffer));

  return buffer;
}

/**
 * Converts the packet header back into a byte stream */
void buffer_header(struct header *header, byte **buffer) {
  size_t inc = sizeof(uint16_t) / sizeof(byte);

  uint16_t id = htons(header->id);
  memcpy(*buffer, &id, inc), *buffer += inc;

  uint16_t flags = htons(header->flags);
  memcpy(*buffer, &flags, inc), *buffer += inc;

  uint16_t qd_count = htons(header->qd_count);
  memcpy(*buffer, &qd_count, inc), *buffer += inc;
  uint16_t an_count = htons(header->an_count);
  memcpy(*buffer, &an_count, inc), *buffer += inc;
  uint16_t ns_count = htons(header->ns_count);
  memcpy(*buffer, &ns_count, inc), *buffer += inc;
  uint16_t ar_count = htons(header->ar_count);
  memcpy(*buffer, &ar_count, inc), *buffer += inc;
}

/**
 * Converts the packet question back into a byte stream */
void buffer_question(struct question *question, byte **buffer) {
  size_t name_len = strlen(question->name) + 1;
  int start = 0;
  for (int i = 0; i < name_len; ++i) {
    if (question->name[i] == '.' || i == name_len - 1) {
      **buffer = start - i, (*buffer)++;

      for (int j = start; j < i; ++j) {
        **buffer = question->name[j], (*buffer)++;
      }

      start = i + 1;
    }
  }
  **buffer = 0, (*buffer)++;

  size_t inc = sizeof(uint16_t) / sizeof(byte);

  uint16_t type = htons(question->type);
  memcpy(*buffer, &type, inc), *buffer += inc;
  uint16_t qclass = htons(question->qclass);
  memcpy(*buffer, &qclass, inc), *buffer += inc;
}

/**
 * Converts the packet answer back into a byte stream */
void buffer_answer(struct resource *answer, byte **buffer) {
  size_t inc = sizeof(uint16_t) / sizeof(byte);

  uint16_t compressed_name = 49164;
  memcpy(*buffer, &compressed_name, inc), *buffer += inc;

  uint16_t type = htons(answer->type);
  memcpy(*buffer, &type, inc), *buffer += inc;

  uint16_t rclass = htons(answer->rclass);
  memcpy(*buffer, &rclass, inc), *buffer += inc;

  uint32_t ttl = htonl(answer->ttl);
  memcpy(*buffer, &ttl, 2 * inc), *buffer += 2 * inc;

  uint16_t rd_length = htons(16);
  memcpy(*buffer, &rd_length, inc), *buffer += inc;

  byte rdata[16];
  inet_pton(AF_INET6, answer->address, rdata);
  memcpy(*buffer, rdata, 16), *buffer += 16;
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
 * Returns 1 if the packet contains an answer, else returns 0 */
int contains_answer(struct packet *packet) { return packet->header.an_count; }

/**
 * Sets the rcode in the header */
void set_rcode(int rcode, struct header *header) { header->flags |= rcode; }

/**
 * Sets the QR bit to 1 */
void set_response(struct header *header) { header->flags |= 1 << 15; }

/**
 * Sets the RA bit to 1 */
void set_RA(struct header *header) { header->flags |= 1 << 7; }

/**
 * Sets all the counts in the header to 0 */
void zero_counts(struct header *header) {
  header->qd_count = 0;
  header->an_count = 0;
  header->ns_count = 0;
  header->ar_count = 0;
}
