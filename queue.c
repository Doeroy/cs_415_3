#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

// Function to create a new node
Node* newNode(int new_data) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = new_data;
    node->next = NULL;
    return node;
}

// Function to initialize the queue
Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}

// Function to check if the queue is empty
int isEmpty(Queue* q) {
    return q->front == NULL;
}

// Function to add an element to the queue
void enqueue(Queue* q, int new_data) {
    Node* new_node = newNode(new_data);
    if (isEmpty(q)) {
        q->front = q->rear = new_node;
        return;
    }
    q->rear->next = new_node;
    q->rear = new_node;
}

// Function to remove an element from the queue
void dequeue(Queue* q) {
    if (isEmpty(q)) {
        return;
    }
    Node* temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(temp);
    //printQueue(q);
}

// Function to print the current state of the queue
void printQueue(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        return;
    }
    Node* temp = q->front;
    printf("Current Queue: ");
    while (temp != NULL) {
        printf("%d ", temp->data);
        temp = temp->next;
    }
    printf("\n");
}
