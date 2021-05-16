#include "log.h"

#include <stdlib.h>
#include <time.h>

#define TIME_LEN 50
/**
 * Opens the log file for writing */
FILE *open_log() {
  FILE *log = fopen("dns_svr.log", "w");
  if (!log) {
    perror("opening log");
    exit(EXIT_FAILURE);
  }

  return log;
}

/**
 * Closes the log file */
void close_log(FILE *log) { fclose(log); }

/**
 * Writes a request message to the log */
void write_request(FILE *log, char *msg) {
  time_t rawtime = time(NULL);
  struct tm *timeinfo = gmtime(&rawtime);

  char time_buf[TIME_LEN];
  strftime(time_buf, TIME_LEN, "%FT%T%z", timeinfo);
  fprintf(log, "%s requested %s\n", time_buf, msg);
  fflush(log);
}
