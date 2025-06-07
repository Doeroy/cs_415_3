#include "queue.h"
#include <string.h>

void queue_init(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
    memset(q->items, 0, sizeof(q->items));
}
int queue_is_full(Queue* q) { return q->count == MAX_QUEUE_SIZE; }
int queue_is_empty(Queue* q) { return q->count == 0; }
void enqueue(Queue* q, int item) {
    if (!queue_is_full(q)) {
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
        q->items[q->rear] = item;
        q->count++;
    }
}
int dequeue(Queue* q) {
    if (!queue_is_empty(q)) {
        int data = q->items[q->front];
        q->front = (q->front + 1) % MAX_QUEUE_SIZE;
        q->count--;
        return data;
    }
    return -1;
}
void print_queue(Queue* q) {
    if (queue_is_empty(q)) { printf("[]"); return; }
    printf("[");
    for (int i = 0; i < q->count; i++) {
        printf("%d%s", q->items[(q->front + i) % MAX_QUEUE_SIZE], i == q->count - 1 ? "" : ", ");
    }
    printf("]");
}