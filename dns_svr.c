#include "list.h"
#include "log.h"
#include "packet.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CACHE
#define MAX_CACHE 5

#define PORT "8053"
#define BACKLOG 5

#define UNIMPL_RCODE 4
#define HEADER_LEN 12

/**
 * Initialises a socket so our server can receive clients */
void init_socket(int *sock_fd, struct addrinfo *hints) {
  int status, yes = 1;
  struct addrinfo *res, *p;

  if ((status = getaddrinfo(NULL, PORT, hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }

  // bind to the first actual result
  for (p = res; p != NULL; p = p->ai_next) {
    if ((*sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      perror("dns_svr: socket");
      continue;
    }

    // allow multiple connections to socket
    if (setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) ==
        -1) {
      perror("dns_svr: setsockopt");
      exit(EXIT_FAILURE);
    }

    if (bind(*sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(*sock_fd);
      perror("dns_svr: bind");
      exit(EXIT_FAILURE);
    }

    break;
  }

  freeaddrinfo(res);

  if (!p) {
    perror("dns_svr: failed to bind");
    exit(EXIT_FAILURE);
  }

  if (listen(*sock_fd, BACKLOG) == -1) {
    perror("dns_svr: listen");
    exit(EXIT_FAILURE);
  }
}

/**
 * Initialises the upstream server's address info */
void init_server(char *addr, char *port, struct addrinfo *hints,
                 struct addrinfo **server_addr) {
  int status;

  if ((status = getaddrinfo(addr, port, hints, server_addr)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }
}

/**
 * Opens a connection to the upstream server and saves the server address */
void connect_to_server(int *server_fd, struct addrinfo **server_addr) {
  struct addrinfo *p;

  for (p = *server_addr; p != NULL; p = p->ai_next) {
    if ((*server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      perror("dns_svr: server socket");
      continue;
    }

    if (connect(*server_fd, p->ai_addr, p->ai_addrlen) != -1) {
      break;
    }

    close(*server_fd);
  }

  if (!p) {
    perror("failed to connect");
    exit(EXIT_FAILURE);
  }

  *server_addr = p;
}

void reconnect_to_server(int *server_fd, struct addrinfo **server_addr) {
  struct addrinfo *p = *server_addr;

  if ((*server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
      -1) {
    perror("dns_svr: reopen server socket");
    exit(EXIT_FAILURE);
  }

  if ((connect(*server_fd, p->ai_addr, p->ai_addrlen)) == -1) {
    perror("dns_svr: reconnect to server");
    exit(EXIT_FAILURE);
  }
}

/**
 * Blocks until a client tries to access the socket and then opens a connection
 * with that client */
void accept_client(int *new_fd, int listener_fd) {
  struct sockaddr_storage client_addr;
  socklen_t addr_size = sizeof client_addr;

  if ((*new_fd = accept(listener_fd, (struct sockaddr *)&client_addr,
                        &addr_size)) == -1) {
    perror("dns_svr: accept");
    exit(EXIT_FAILURE);
  }
}

/**
 * Reads from the socket until all bytes have been received */
ssize_t read_all(int sock_fd, void *buffer, uint16_t size) {
  ssize_t n;
  size_t recv = 0;

  while (recv < size) {
    n = read(sock_fd, buffer + recv, size - recv);
    if (n == 0)
      return n;
    if (n < 0) {
      perror("read all");
      exit(EXIT_FAILURE);
    }

    recv += n;
  }

  return recv;
}

/**
 * Reads a DNS packet from the specificed socket */
ssize_t read_packet(int sock_fd, byte **buffer, uint16_t *buf_size) {
  uint16_t tbuf_size;
  if (!read_all(sock_fd, &tbuf_size, 2)) {
    return 0;
  }

  *buf_size = ntohs(tbuf_size);

  *buffer = malloc(*buf_size);
  assert(*buffer);

  return read_all(sock_fd, *buffer, *buf_size);
}

/**
 * Writes a DNS packet to the specified socket */
void write_packet(int sock_fd, byte *buffer, uint16_t buf_size) {
  ssize_t n, m;
  uint16_t nbuf_size = htons(buf_size);

  n = write(sock_fd, &nbuf_size, 2);
  if (n < 0) {
    perror("write packet");
    exit(EXIT_FAILURE);
  }

  m = write(sock_fd, buffer, buf_size);
  if (m < 0) {
    perror("write packet");
    exit(EXIT_FAILURE);
  }

  if (n + m < buf_size) {
    fprintf(stderr, "Only wrote %ld of %d bytes\n", (n + m), buf_size);
    exit(EXIT_FAILURE);
  }
}

/**
 * Writes the packet header through the specified socket */
void send_packet_header(int sock_fd, struct header *header) {
  // Set header flags
  set_rcode(UNIMPL_RCODE, header);
  set_response(header);
  set_RA(header);
  zero_counts(header);

  uint16_t nlen = htons(HEADER_LEN);
  write(sock_fd, &nlen, sizeof(uint16_t));

  byte *buffer = malloc(HEADER_LEN);
  assert(buffer);

  byte *buffer_ptr = buffer;
  buffer_header(header, &buffer_ptr);
  write(sock_fd, buffer, HEADER_LEN);

  free(buffer);
}

void send_packet(int sock_fd, int id, struct packet *packet) {
  uint16_t buf_size = htons(packet->size);
  write(sock_fd, &buf_size, sizeof(uint16_t));

  packet->header.id = id;

  byte *buffer = buffer_packet(packet);

  write(sock_fd, buffer, packet->size);

  free(buffer);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: ./dns_svr addr port\n");
    exit(EXIT_FAILURE);
  }

  // set up socket for clients
  struct addrinfo hints;
  int listener_fd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  init_socket(&listener_fd, &hints);

  // set up socket for forwarding requests
  struct addrinfo *server_addr;
  int server_fd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  init_server(argv[1], argv[2], &hints, &server_addr);
  connect_to_server(&server_fd, &server_addr);

  struct packet *packet = NULL;
  byte *req_buf, *res_buf;
  uint16_t buf_size;
  int sock_fd;

  FILE *log = open_log();
  linked_list_t *cache = new_list();
  node_t *result;

  while (1) {
    accept_client(&sock_fd, listener_fd);

    // Parse request from client
    read_packet(sock_fd, &req_buf, &buf_size);
    packet = parse_packet(req_buf, buf_size);

    log_request(log, packet->question.name);

    if (!is_AAAA_request(packet)) {
      log_invalid_request(log);

      send_packet_header(sock_fd, &packet->header);

      free(req_buf);
      free_packet(packet);
      close(sock_fd);
      continue;
    }

    if (!empty_list(cache) &&
        search_list(cache, packet->question.name, &result)) {
      uint32_t ttl = get_ttl(result);
      if (ttl != 0) {
        log_cache_hit(log, packet->question.name, ttl);

        // set the id
        memcpy(result->buffer, &packet->header.id, 2);
        uint16_t nbuf = htons(result->buf_size);
        write(sock_fd, &nbuf, 2);
        write(sock_fd, result->buffer, result->buf_size);

        move_front_list(cache, result);

        free(req_buf);
        free_packet(packet);
        close(sock_fd);
        continue;
      } else {
        delete_list(cache, result);
      }
    }

    // forward to server
    write_packet(server_fd, req_buf, buf_size);

    free_packet(packet);

    // Parse response from server
    if (!read_packet(server_fd, &res_buf, &buf_size)) {
      close(server_fd);
      reconnect_to_server(&server_fd, &server_addr);

      write_packet(server_fd, req_buf, buf_size);
      read_packet(server_fd, &res_buf, &buf_size);
    }

    free(req_buf);

    packet = parse_packet(res_buf, buf_size);

    if (contains_answer(packet) && is_AAAA_response(packet)) {
      log_response(log, packet->answer.name, packet->answer.address);

      if (cache->size == MAX_CACHE) {
        delete_expired_list(cache);
        if (cache->size == MAX_CACHE) {
          log_cache_replace(log, cache->tail->name, packet->answer.name);
          delete_list(cache, cache->tail);
        }
      }

      insert_list(cache, packet->answer.name, res_buf, buf_size,
                  packet->answer.ttl);
    }
    write_packet(sock_fd, res_buf, buf_size);

    free(res_buf);
    free_packet(packet);

    close(sock_fd);
  }

  freeaddrinfo(server_addr);

  close_log(log);
  free_list(cache);

  close(server_fd);
  close(listener_fd);

  return 0;
}
