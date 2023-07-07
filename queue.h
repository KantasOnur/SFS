#ifndef QUEUE_H
#define QUEUE_H

struct node {
    char name[50];
    int flc;
    struct node *next;
};

struct queue {
    struct node *front;
    struct node *rear;
};

void initQueue(struct queue *q);
int isEmpty(struct queue *q);
void enqueue(struct queue *q, char name[], int flc);
struct node *dequeue(struct queue *q);
void printQueue(struct queue *q);

#endif /* QUEUE_H */

