#ifndef __LIST_H_
#define __LIST_H_

#include "packet.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct list_node node_t;

struct list_node {
  struct packet *packet;
  char *name;
  char *address;
  time_t last_used;
  node_t *prev;
  node_t *next;
};

typedef struct linked_list {
  node_t *head;
  node_t *tail;
  uint32_t size;
} linked_list_t;

linked_list_t *new_list();
void free_list(linked_list_t *list);

bool empty_list(linked_list_t *list);

bool search_list(linked_list_t *list, char *name, node_t **result);
void insert_list(linked_list_t *list, struct packet *packet);
void move_front_list(linked_list_t *list, node_t *node);
bool find_expired_list(linked_list_t *list, node_t **result);
void delete_list(linked_list_t *list, node_t *node);

uint32_t get_ttl(node_t *node);

void print_list(linked_list_t *list);

#endif // __LIST_H_
