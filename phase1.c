// For testing logging/parsing DNS requests

#include "log.h"
#include "packet.h"

int main(int argc, char *argv[]) {
  FILE *log = open_log();
  struct packet *packet = parse_packet(0);

  if (is_response(packet)) {
    write_response(log, packet->answer.name, packet->answer.address);
  } else {
    write_request(log, packet->question.name);
    if (!is_AAAA(packet)) {
      write_invalid_request(log);
    }
  }

  free_packet(packet);
  close_log(log);

  return 0;
}
