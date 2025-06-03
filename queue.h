#include <stdio.h>
#include <stdlib.h>

typedef struct Node{
    int data;
    struct Node* next;
} Node;

typedef struct Queue{
    struct Node* front;
    struct Node* rear;
} Queue;

Node* newNode(int new_data);

Queue* createQueue();

int isEmpty(struct Queue* q);

void enqueue(struct Queue* q, int new_data);

void dequeue(struct Queue* q);

void printQueue(struct Queue* q);