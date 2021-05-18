#ifndef LOG_H_
#define LOG_H_

#include <stdint.h>
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
 * Writes an invalid request to the log */
void log_invalid_request(FILE *log);

/**
 * Writes a response message to the log */
void log_response(FILE *log, char *domain, char *address);

/**
 * Writes the cache entry and its expiry to the log */
void log_cache_hit(FILE *log, char *domain, uint32_t ttl);

/**
 * Writes a cache replacement to the log */
void log_cache_replace(FILE *log, char *old, char *update);

#endif // LOG_H_
