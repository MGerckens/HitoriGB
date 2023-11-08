#include "queues.h"

#include <stdint.h>
#include <stdlib.h>

void enqueue(node_t **head, uint8_t val) {
    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node) return;

    new_node->val = val;
    new_node->next = *head;

    *head = new_node;
}

uint8_t dequeue(node_t **head) {
    node_t *current, *prev = NULL;
    uint8_t retval = 255;  // game currently caps at 15x15, 224 maximum

    if (*head == NULL) {
        return retval;
    }

    current = *head;
    while (current->next != NULL) {
        prev = current;
        current = current->next;
    }

    retval = current->val;

    free(current);

    if (prev)
        prev->next = NULL;
    else
        *head = NULL;

    return retval;
}