#ifndef QUEUE_H
#define QUEUE_H

typedef struct node {
    uint8_t val;
    struct node *next;
} node_t;
extern void enqueue(node_t **head, uint8_t val);
extern uint8_t dequeue(node_t **head);

#endif //QUEUE_H