#ifndef __TINYFILE_SERVICE_H
#define __TINYFILE_SERVICE_H

#include <stdlib.h>

#define NUM_THREADS 5
#define MAX_THREADS 8

typedef struct segment {
    int msg_id;
    int seg_id;
} segment_t;

typedef struct queue_data {
    int msg_id;
    struct queue* queue;
} queue_data_t;

extern int open[MAX_THREADS];
extern int server_shm_size;

struct reqnode {
    int tid;
    struct reqnode* next;
};

struct reqnode_list {
    int size;
    struct reqnode* head;
    struct reqnode* tail;
};

struct reqnode* create_reqnode(int val) {
    struct reqnode* node = (struct reqnode *)(malloc(sizeof(struct reqnode)));
    node->tid = val;
    node->next = NULL;
    return node;
}

void add_reqnode(int val, struct reqnode_list* list) {
    struct reqnode* node = create_reqnode(val);
    if (list->size == 0) {
        list->head = list->tail = node;
        list->size++;
    } else {
        list->tail->next = node;
        list->tail = node;
        list->size++;
    }
    return;
}

int pop_reqnode(struct reqnode_list* list) {
    if (list->size == 0) {
        return -1;
    } else {
        int val = list->head->tid;
        struct reqnode* tmp = list->head;
        list->head = list->head->next;
        free(tmp);
        list->size--;
        return val;
    }
}

struct qnode {
    int client_id;
    struct reqnode_list* list;
    struct qnode* next;
};

struct queue {
    int size;
    struct qnode* head;
    struct qnode* tail;
};

struct qnode* create_qnode(int id, struct reqnode_list* list) {
    struct qnode* node = (struct qnode*)(malloc(sizeof(struct qnode)));
    node->list = list;
    node->client_id = id;
    node->next = NULL;
    return node;
}

void enqueue(int id, struct reqnode_list* list, struct queue* queue) {
    struct qnode* node = create_qnode(id, list);
    if (queue->size == 0) {
        queue->head = queue->tail = node;
        queue->size++;
    } else {
        queue->tail->next = node;
        queue->tail = node;
        queue->tail = queue->head;
        queue->size++;
    }
    return;
}

int reqget(struct queue* queue) {
    if (queue->size == 0) {
        return -1;
    } else {
        if (queue->head->list->size == 0) {
            queue->size--;
            struct qnode* tmp = queue->head;
            if (queue->size == 0) {
                queue->head = queue->tail = NULL;
                free(tmp->list);
                free(tmp);
                return -1;
            } else if (queue->size == 1) {
                queue->head = queue->tail = queue->head->next;
                queue->tail->next = queue->head;
                free(tmp->list);
                free(tmp);
            } else {
                queue->head = queue->head->next;
                queue->tail->next = queue->head;
                free(tmp->list);
                free(tmp);
            }
            return reqget(queue);
        } else {
            int client_tid = pop_reqnode(queue->head->list);
            queue->head = queue->head->next;
            return client_tid;
        }
    }
}

#endif