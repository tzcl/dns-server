#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

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
  char *name;
  uint16_t type;   // assume to be AAAA records (value of 28)
  uint16_t qclass; // assume to IN (value of 1)
};

struct resource {
  char *name;
  uint16_t type;
  uint16_t rclass;
  uint32_t ttl;
  uint16_t rd_length;
  byte *rdata;
  char *address;
};

struct packet {
  struct header header;
  struct question question;
  struct resource answer;
  byte *remaining;
};

/**
 * Creates the memory for a packet */
struct packet *init_packet();

/**
 * Frees the memory allocated to a packet */
void free_packet(struct packet *packet);

struct packet *parse(byte buffer[], int n);

/**
 * Reads in the next message from the given file descriptor.
 * Caller is responsible for freeing the resulting struct.
 */
struct packet *parse_packet(int fd);

/**
 * Populates the packet header and increments the buffer pointer */
void parse_header(struct packet *packet, byte **buffer);

/**
 * Populates the packet question and increments the buffer pointer */
void parse_question(struct packet *packet, byte **buffer);

/**
 * Populates the packet answer and increments the buffer pointer */
void parse_answer(struct packet *packet, byte **buffer);

/**
 * Returns 1 if the packet is a response, else returns 0  */
int is_response(struct packet *packet);

/**
 * Returns 1 if the request is for an AAAA record, else returns 0
 * Assumes the packet is known to be a DNS request */
int is_AAAA(struct packet *packet);

#endif // PACKET_H_
