#ifndef QUEUE_H
#define QUEUE_H

struct Queue {
    int front, rear, size;
    unsigned capacity;
    char** array; // Pointer to an array of strings
};

// Function prototypes
struct Queue* createQueue(unsigned capacity);
int isFull(struct Queue* queue);
int isEmpty(struct Queue* queue);
void enqueue(struct Queue* queue, char* item);
char* dequeue(struct Queue* queue);
char* front(struct Queue* queue);
char* rear(struct Queue* queue);

#endif // QUEUE_H
