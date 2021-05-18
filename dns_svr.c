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

void accept_client(int *new_fd, int listener_fd) {
  struct sockaddr_storage client_addr;
  socklen_t addr_size = sizeof client_addr;

  if ((*new_fd = accept(listener_fd, (struct sockaddr *)&client_addr,
                        &addr_size)) == -1) {
    perror("dns_svr: accept");
    exit(EXIT_FAILURE);
  }
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
  struct addrinfo *res, *p;
  int forward_fd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int status;
  if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }

  // TODO: extract into function?
  for (p = res; p != NULL; p = p->ai_next) {
    forward_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (forward_fd == -1)
      continue;

    if (connect(forward_fd, p->ai_addr, p->ai_addrlen) != -1) {
      break;
    }

    close(forward_fd);
  }

  if (!p) {
    perror("failed to connect");
    exit(EXIT_FAILURE);
  }

  printf("server: waiting for connections...\n");

  printf("server: received connection!\n");

  struct packet *packet = NULL;
  byte buffer[2048];
  int sock_fd, n;
  while (1) {
    accept_client(&sock_fd, listener_fd);
    // TODO: need to refactor packet parsing
    /* packet = parse_packet(sock_fd); */
    n = read(sock_fd, buffer, 2048);
    if (n < 0) {
      perror("failed to read");
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; ++i) {
      printf("%x ", buffer[i]);
    }
    printf("\n");

    // forward to server
    n = write(forward_fd, buffer, n);
    printf("wrote %d bytes to server\n", n);
    if (n == 0) {
      fprintf(stderr, "Not writing anything!!!!\n");
      exit(EXIT_FAILURE);
    }
    if (n < 0) {
      perror("failed to write");
      exit(EXIT_FAILURE);
    }

    int old_n = n;

    n = read(forward_fd, buffer, 2048);
    printf("read %d bytes from server\n", n);
    if (n == 0) {
      // TODO: server closed the connection!
      close(forward_fd);
      // reusing res from before
      // TODO: extract into function?
      for (p = res; p != NULL; p = p->ai_next) {
        forward_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (forward_fd == -1)
          continue;

        if (connect(forward_fd, p->ai_addr, p->ai_addrlen) != -1) {
          break;
        }

        close(forward_fd);
      }

      if (!p) {
        perror("failed to connect");
        exit(EXIT_FAILURE);
      }

      printf("Reconnecting to the upstream server...\n");

      n = write(forward_fd, buffer, old_n);
      printf("wrote %d bytes to server\n", n);
      n = read(forward_fd, buffer, 2048);
      printf("read %d bytes from server\n", n);
    }
    if (n < 0) {
      perror("failed to read");
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; ++i) {
      printf("%x ", buffer[i]);
    }
    printf("\n");

    n = write(sock_fd, buffer, n);
    printf("wrote %d bytes to client\n", n);
    if (n < 0) {
      perror("failed to write");
      exit(EXIT_FAILURE);
    }

    close(sock_fd);

    printf("DONE!\n");
    printf("----------------------------------------------------\n");
    memset(buffer, 0, 2048);
  }

  freeaddrinfo(res);

  close(sock_fd);

  close(forward_fd);
  close(listener_fd);

  return 0;
}
