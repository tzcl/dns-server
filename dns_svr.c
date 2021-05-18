#include "log.h"
#include "packet.h"

#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// TODO: refactor time variable from log to this class?

#define PORT "8053"
#define BACKLOG 5

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
 * Reads a DNS packet from the specificed socket */
void read_packet(int sock_fd, byte **buffer, uint16_t *buf_size) {
  ssize_t n;

  // TODO: implement a read_all function?
  n = read(sock_fd, buf_size, 2);
  if (n <= 0) {
    perror("failed to read from client");
    exit(EXIT_FAILURE);
  }
  *buf_size = ntohs(*buf_size);

  *buffer = malloc(*buf_size);
  if (!buffer) {
    perror("allocating buffer");
    exit(EXIT_FAILURE);
  }

  n = read(sock_fd, *buffer, *buf_size);
  if (n <= 0) {
    perror("failed to read from client");
    exit(EXIT_FAILURE);
  }
}

/**
 * Writes a DNS packet to the specified socket */
void write_packet(int sock_fd, byte *buffer, uint16_t buf_size) {}

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

  byte *buffer;
  uint16_t buf_size, nbuf_size;
  int sock_fd, n;

  while (1) {
    accept_client(&sock_fd, listener_fd);

    read_packet(sock_fd, &buffer, &buf_size);

    printf("Buffer size %d\n", buf_size);
    for (int i = 0; i < buf_size; ++i) { // TODO: debug
      printf("%x ", buffer[i]);
    }
    printf("\n");

    // forward to server
    nbuf_size = htons(buf_size);
    n = write(server_fd, &nbuf_size, 2);
    n += write(server_fd, buffer, buf_size);
    printf("wrote %d bytes to server\n", n); // TODO: debug
    if (n < 0) {
      perror("failed to write to server");
      exit(EXIT_FAILURE);
    }

    free(buffer);

    read_packet(server_fd, &buffer, &buf_size);
    printf("read %d bytes from server\n", buf_size); // TODO: debug
    // TODO: move this into read_packet??
    if (n == 0) {
      // upstream server closed the connection!
      close(server_fd);

      printf("Reconnecting to the upstream server...\n"); // TODO: debug
      reconnect_to_server(&server_fd, &server_addr);
      printf("Reconnected to the upstream server!\n"); // TODO: debug

      // resend buffer to upstream server
      /* n = write(server_fd, buffer, n); */
      printf("wrote %d bytes to server\n", n); // TODO: debug
      /* n = read(server_fd, buffer, buf_); */
      printf("read %d bytes from server\n", n); // TODO: debug
    }
    if (n < 0) {
      perror("failed to read from server");
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < buf_size; ++i) { // TODO: debug
      printf("%x ", buffer[i]);
    }
    printf("\n");

    nbuf_size = htons(buf_size);
    n = write(sock_fd, &nbuf_size, 2);
    n += write(sock_fd, buffer, buf_size);
    printf("wrote %d bytes to client\n", n); // TODO: debug
    if (n < 0) {
      perror("failed to write to client");
      exit(EXIT_FAILURE);
    }

    // finished communicating with the client
    free(buffer);
    close(sock_fd);

    printf("DONE!\n");
    printf("----------------------------------------------------\n"); // TODO:
    // debug
    break;
  }

  freeaddrinfo(server_addr);

  close(server_fd);
  close(listener_fd);

  return 0;
}
