#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Queue* createQueue(unsigned capacity) {
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    if (!queue) return NULL;
    
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;
    queue->array = (char**)malloc(queue->capacity * sizeof(char*));
    if (!queue->array) {
        free(queue);
        return NULL;
    }
    
    return queue;
}

int isFull(struct Queue* queue) {
    return (queue->size == queue->capacity);
}

int isEmpty(struct Queue* queue) {
    return (queue->size == 0);
}

void enqueue(struct Queue* queue, char* item) {
    if (isFull(queue)) return;
    
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = strdup(item); // Copy the string into the queue
    queue->size = queue->size + 1;
    printf("%s enqueued to queue\n", item);
}

char* dequeue(struct Queue* queue) {
    if (isEmpty(queue)) return NULL;
    
    char* item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

char* front(struct Queue* queue) {
    if (isEmpty(queue)) return NULL;
    return queue->array[queue->front];
}

char* rear(struct Queue* queue) {
    if (isEmpty(queue)) return NULL;
    return queue->array[queue->rear];
}
