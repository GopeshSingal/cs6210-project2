#ifndef __TINYFILE_SERVICE_H
#define __TINYFILE_SERVICE_H

#include <stdlib.h>

#define NUM_THREADS 5
#define MAX_THREADS 8

typedef struct segment {
    int msg_id;
    int seg_id;
} segment_t;

typedef struct queue_thread_data {
    int msg_id;
    struct client_queue* queue;
} queue_thread_data_t;

struct qnode {
    int recv_id;
    struct qnode* next;
};

struct client_queue {
    struct qnode* front;
    struct qnode* rear;
    int size;
};

struct qnode* create_node(int id) {
    struct qnode* new_node = (struct qnode*)(malloc(sizeof(struct qnode)));
    new_node->recv_id = id;
    new_node->next = NULL;
    return new_node;
}

struct client_queue* create_queue() {
    struct client_queue* queue = (struct client_queue*) malloc(sizeof(struct client_queue));
    queue->front = queue->rear = NULL;
    queue->size = 0;
    return queue;
}

void enqueue(struct client_queue* queue, int id) {
    struct qnode* node = create_node(id);
    if (queue->rear == NULL) {
        queue->front = queue->rear = node;
        queue->rear->next = queue->front;
    } else {
        queue->rear->next = node;
        queue->rear = node;
        queue->rear->next = queue->front;
    }
    queue->size++;
}

int dequeue(struct client_queue* queue) {
    int return_id;
    if (queue->size == 0) {
        return -1;
    }
    if (queue->front == queue->rear) {
        return_id = queue->front->recv_id;
        free(queue->front);
        queue->front = queue->rear = NULL;
    } else {
        struct qnode* tmp = queue->front;
        return_id = tmp->recv_id;
        queue->front = queue->front->next;
        queue->rear->next = queue->front;
        free(tmp);
    }
    queue->size--;
    return return_id;
}

extern int open[MAX_THREADS];
extern int server_shm_size;

#endif