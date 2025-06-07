#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#define MAX_QUEUE_SIZE 100

typedef struct {
    int items[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
} Queue;

void queue_init(Queue* q);
int queue_is_full(Queue* q);
int queue_is_empty(Queue* q);
void enqueue(Queue* q, int item);
int dequeue(Queue* q);
void print_queue(Queue* q);

#endif