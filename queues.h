#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

typedef struct node {
    uint8_t val;
    struct node *next;
} node_t;
void enqueue(node_t **head, uint8_t val);
uint8_t dequeue(node_t **head);

#endif  // QUEUE_H