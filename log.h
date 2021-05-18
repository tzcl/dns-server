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
void log_request(FILE *log, char *domain);

/**
 * Writes that there has been an invalid request in the log */
void log_invalid_request(FILE *log);

/**
 * Writes a response message to the log */
void log_response(FILE *log, char *domain, char *address);

#endif // LOG_H_
