// For testing logging/parsing DNS requests

#include "packet.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
  struct packet *packet = parse_packet(0);

  // can append the rest of the buffer?
  // or do I need to parse the AR record?

  // LOGGING
  FILE *log = fopen("dns_svr.log", "w");
  if (!log) {
    perror("opening log");
    exit(EXIT_FAILURE);
  }

  time_t rawtime = time(NULL);
  struct tm *timeinfo = gmtime(&rawtime);

  char time_buf[256];
  strftime(time_buf, 256, "%FT%T%z", timeinfo);
  fprintf(log, "%s requested %s\n", time_buf, packet->question.name);
  fflush(log);

  fclose(log);

  return 0;
}
