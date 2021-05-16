// For testing logging/parsing DNS requests

#include "log.h"
#include "packet.h"

int main(int argc, char *argv[]) {
  FILE *log = open_log();
  struct packet *packet = parse_packet(0, log);

  free_packet(packet);
  close_log(log);

  return 0;
}
