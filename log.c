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
 * Writes a string containing the current timestamp to the given buffer */
void get_time(char *buffer) {
  time_t rawtime = time(NULL);
  struct tm *timeinfo = gmtime(&rawtime);
  strftime(buffer, TIME_LEN, "%FT%T%z", timeinfo);
}

/**
 * Writes a request message to the log */
void write_request(FILE *log, char *domain) {
  char time_buf[TIME_LEN];
  get_time(time_buf);
  fprintf(log, "%s requested %s\n", time_buf, domain);
  fflush(log);
}

/**
 * Writes that there has been an invalid request in the log */
void write_invalid_request(FILE *log) {
  char time_buf[TIME_LEN];
  get_time(time_buf);
  fprintf(log, "%s unimplemented request\n", time_buf);
  fflush(log);
}

/**
 * Writes a response message to the log */
void write_response(FILE *log, char *domain, char *address) {
  char time_buf[TIME_LEN];
  get_time(time_buf);
  fprintf(log, "%s %s is at %s\n", time_buf, domain, address);
  fflush(log);
}
