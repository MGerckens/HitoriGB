
typedef struct node {
    uint8_t val;
    struct node *next;
} node_t;


void enqueue(node_t **head, uint8_t val) {
    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node) return;

    new_node->val = val;
    new_node->next = *head;

    *head = new_node;
}

uint8_t dequeue(node_t **head) {
    node_t *current, *prev = NULL;
    uint8_t retval =255; //unlikely to get this big on a GB screen

    if (*head == NULL) { return retval; }

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