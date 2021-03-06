#include "list.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TIME_LEN 16

node_t *new_list_node(struct packet *packet) {
  node_t *node = malloc(sizeof(node_t));
  assert(node);

  node->packet = packet;

  node->name = packet->answer.name;
  node->address = packet->answer.address;
  node->last_used = time(NULL);

  node->prev = NULL;
  node->next = NULL;

  return node;
}

linked_list_t *new_list() {
  linked_list_t *list = malloc(sizeof(linked_list_t));
  assert(list);

  list->size = 0;
  list->head = NULL;
  list->tail = NULL;

  return list;
}

void free_list(linked_list_t *list) {
  node_t *curr = list->head, *prev;
  while (curr) {
    prev = curr;
    curr = curr->next;
    free_packet(prev->packet);
    free(prev);
  }

  free(list);
}

bool empty_list(linked_list_t *list) { return list->size == 0; }

bool search_list(linked_list_t *list, char *name, node_t **result) {
  assert(list);
  if (empty_list(list))
    return false;

  node_t *curr = list->head;
  while (curr) {
    if (strcmp(name, curr->packet->answer.name) == 0) {
      *result = curr;
      return true;
    }
    curr = curr->next;
  }

  return false;
}

void insert_list(linked_list_t *list, struct packet *packet) {
  node_t *node = new_list_node(packet);
  assert(node && list);

  if (list->head) {
    list->head->prev = node;
    node->next = list->head;
  }
  if (!list->tail) {
    list->tail = node;
  }

  list->head = node;
  list->size++;
}

void move_front_list(linked_list_t *list, node_t *node) {
  assert(node && list);

  if (node == list->head) {
    list->head = node->next;
  }
  if (node == list->tail) {
    list->tail = node->prev;
  }

  if (node->prev) {
    node->prev->next = node->next;
    node->prev = NULL;
  }
  if (node->next) {
    node->next->prev = node->prev;
  }

  if (list->head) {
    list->head->prev = node;
    node->next = list->head;
  }
  if (!list->tail) {
    list->tail = node;
  }

  list->head = node;
}

void delete_list(linked_list_t *list, node_t *node) {
  assert(node && list);

  if (node == list->head) {
    list->head = node->next;
  }
  if (node == list->tail) {
    list->tail = node->prev;
  }

  if (node->prev) {
    node->prev->next = node->next;
  }
  if (node->next) {
    node->next->prev = node->prev;
  }

  free(node);
  list->size--;
}

bool find_expired_list(linked_list_t *list, node_t **result) {
  node_t *curr = list->tail;
  while (curr) {
    if (get_ttl(curr) == 0) {
      *result = curr;
      return true;
    }
    curr = curr->prev;
  }

  return false;
}

uint32_t get_ttl(node_t *node) {
  time_t now = time(NULL);

  double elapsed = difftime(now, node->last_used);
  node->last_used = now;

  node->packet->answer.ttl = elapsed > node->packet->answer.ttl
                                 ? 0
                                 : node->packet->answer.ttl - elapsed;

  return node->packet->answer.ttl;
}

void print_list(linked_list_t *list) {
  char buffer[TIME_LEN];
  struct tm *tm_info;

  node_t *curr = list->head;
  while (curr) {
    tm_info = localtime(&curr->last_used);
    strftime(buffer, TIME_LEN, "%H:%M:%S", tm_info);
    printf("%s --> %s\n", curr->packet->answer.name,
           curr->packet->answer.address);
    printf("  TTL: %d (retrieved: %s)\n", get_ttl(curr), buffer);
    curr = curr->next;
  }
  printf("\n");
}
