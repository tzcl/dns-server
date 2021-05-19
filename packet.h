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
  uint16_t original;
  char *name;
  uint16_t type;
  uint16_t rclass;
  uint32_t ttl;
  char *address;
};

struct packet {
  uint16_t size;
  struct header header;
  struct question question;
  struct resource answer;
  byte *remaining;
};

/**
 * Frees the memory allocated to a packet */
void free_packet(struct packet *packet);

/**
 * Parse the packet in the given buffer.
 * Caller is responsible for freeing the resulting struct.
 */
struct packet *parse_packet(byte *buffer, uint16_t buf_size);

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
 * Converts the packet back into a byte stream.
 * Caller is responsible for freeing the resulting buffer */
byte *buffer_packet(struct packet *packet);

/**
 * Converts the packet header back into a byte stream */
void buffer_header(struct header *packet, byte **buffer);

/**
 * Converts the packet question back into a byte stream */
void buffer_question(struct question *packet, byte **buffer);

/**
 * Converts the packet answer back into a byte stream */
void buffer_answer(struct resource *packet, byte **buffer);

/**
 * Returns 1 if the packet is a response, else returns 0  */
int is_response(struct packet *packet);

/**
 * Returns 1 if the request is for an AAAA record, else returns 0
 */
int is_AAAA_request(struct packet *packet);

/**
 * Returns 1 if the first answer is an AAAA field, else returns 0
 */
int is_AAAA_response(struct packet *packet);

/**
 * Returns 1 if the packet contains an answer, else returns 0 */
int contains_answer(struct packet *packet);

/**
 * Sets the rcode in the header */
void set_rcode(int rcode, struct header *header);

/**
 * Sets the QR bit to 1 */
void set_response(struct header *header);

/**
 * Sets the RA bit to 1 */
void set_RA(struct header *header);

/**
 * Sets all the counts in the header to 0 */
void zero_counts(struct header *header);

#endif // PACKET_H_
