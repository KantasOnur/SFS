#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

void initQueue(struct queue *q) {
    q->front = NULL;
    q->rear = NULL;
}

int isEmpty(struct queue *q) {
    if (q->rear == NULL) {
        return 1;
    } else {
        return 0;
    }
}

void enqueue(struct queue *q, char name[], int flc) {
    struct node *newNode = (struct node *) malloc(sizeof(struct node));
    strcpy(newNode->name, name);
    newNode->flc = flc;
    newNode->next = NULL;

    if (q->rear == NULL) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

struct node* dequeue(struct queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        return NULL;
    } else {
        struct node *temp = q->front;
        q->front = q->front->next;

        if (q->front == NULL) {
            q->rear = NULL;
        }

        return temp;
    }
}

void printQueue(struct queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
    } else {
        struct node *current = q->front;

        while (current != NULL) {
            printf("Name: %s, FLC: %d\n", current->name, current->flc);
            current = current->next;
        }
    }
}

