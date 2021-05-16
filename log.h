#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

/**
 * Opens the log file for writing */
FILE *open_log();

/**
 * Closes the log file */
void close_log(FILE *log);

/**
 * Writes a request message to the log */
void write_request(FILE *log, char *msg);

#endif // LOG_H_
