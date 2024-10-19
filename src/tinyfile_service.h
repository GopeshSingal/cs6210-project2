#ifndef __TINYFILE_SERVICE_H
#define __TINYFILE_SERVICE_H

#include <stdlib.h>

#define NUM_THREADS 5
#define MAX_THREADS 8

#define HASH_MAP_SIZE 10
#define FNV_32_PRIME 16777619u
#define FNV_32_OFFSET_BASIS 2166136261u

typedef struct segment {
    int msg_id;
    int seg_id;
} segment_t;

struct llnode {
    int recv_id;
    struct llnode* next;
};

struct llnode* create_llnode(int id) {
    struct llnode* new_node = (struct llnode*)(malloc(sizeof(struct llnode)));
    new_node->recv_id = id;
    new_node->next = NULL;
    return new_node;
}
typedef struct queue_thread_data {
    int msg_id;
    struct client_queue* queue;
    struct client_queue* fake_queue;
} queue_thread_data_t;

struct qnode {
    int recv_id;
    struct qnode* next;
    struct llnode* head;
    struct llnode* tail;
    int size;
};

struct client_queue {
    struct qnode* front;
    struct qnode* rear;
    int size;
};
typedef struct HashNode {
    int key;
    struct qnode* value;
    struct HashNode* next;
} HashNode;
typedef struct HashMap {
    HashNode* buckets[HASH_MAP_SIZE];
} HashMap;

long fnv1a_hash(long data) {
    long hash = FNV_32_OFFSET_BASIS;
    for (int i = 0; i < 4; i++) {
        hash ^= (data & 0xFF);
        hash *= FNV_32_PRIME;
        data >>= 8;            
    }
    return hash % HASH_MAP_SIZE;
}

void add_llnode(struct qnode* node, int id) {
    if (!node) {
        return;
    }
    node->size++;
    struct llnode* new_node = create_llnode(id);
    if (!node->head) {
        node->head = new_node;
    }
    if (node->tail) {
        node->tail->next = new_node;
    }
    node->tail = new_node;
    return;
}

int remove_llnode(struct qnode* node) {
    if (!node) {
        return -1;
    }
    node->size--;
    int return_id;
    return_id = node->head->recv_id;
    if (!node->head->next) {
        free(node->head);
    } else {
        struct llnode* tmp = node->head;
        node->head = node->head->next;
        free(tmp);
    }
    return return_id;
}

struct qnode* create_node(int id) {
    struct qnode* new_node = (struct qnode*)(malloc(sizeof(struct qnode)));
    new_node->recv_id = id;
    new_node->next = NULL;
    new_node->head = NULL;
    new_node->tail = NULL;
    new_node->size = 0;
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

HashNode* create_hash_node(int key, struct qnode* value) {
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    if (!new_node) {
        printf("Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    new_node->key = key;
    new_node->value = value;
    new_node->next = NULL;
    return new_node;
}

HashMap* create_hash_map() {
    HashMap* map = (HashMap*)malloc(sizeof(HashMap));
    if (!map) {
        printf("Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < HASH_MAP_SIZE; i++) {
        map->buckets[i] = NULL;
    }
    return map;
}

void hash_map_put(HashMap* map, int key, struct qnode* value) {
    unsigned int index = fnv1a_hash(key);
    HashNode* current = map->buckets[index];
    HashNode* prev = NULL;
    while (current != NULL) {
        if (current->key == key) {
            current->value = value;
            return;
        }
        prev = current;
        current = current->next;
    }
    HashNode* new_node = create_hash_node(key, value);
    if (prev == NULL) {
        map->buckets[index] = new_node;
    } else {
        prev->next = new_node;
    }
}

struct qnode* hash_map_get(HashMap* map, int key) {
    unsigned int index = fnv1a_hash(key); 
    HashNode* current = map->buckets[index];
    while (current != NULL) {
        if (current->key == key) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

void hash_map_delete(HashMap* map, int key) {
    unsigned int index = fnv1a_hash(key);
    HashNode* current = map->buckets[index];
    HashNode* prev = NULL;
    while (current != NULL) {
        if (current->key == key) {
            if (prev == NULL) {
                map->buckets[index] = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Fake enqueue adds a node to the respective queue for QoS
struct qnode* fake_enqueue(struct client_queue* queue, int id) {
    struct qnode* node = create_node(id);
    node->head = NULL;
    node->tail = NULL;
    if (queue->rear == NULL) {
        queue->front = queue->rear = node;
        queue->rear->next = queue->front;
    } else {
        queue->rear->next = node;
        queue->rear = node;
        queue->rear->next = queue->front;
    }
    queue->size++;
    return node;
}

// Fake dequeue removes the node from head and rotates queue for QoS
int fake_dequeue(struct client_queue* queue, struct HashMap* q_map) {
    int return_id = -1;
    if (queue->size == 0) {
        return return_id;
    }
    if (queue->front->size > 0) {
        return_id = remove_llnode(queue->front);
    }
    if (queue->front->size == 0) {
        int client_id = queue->front->recv_id;
        hash_map_delete(q_map, client_id);
        dequeue(queue);
    } else {
        struct qnode* tmp = queue->front;
        queue->front = queue->front->next;
        queue->rear = tmp;
    }
    return return_id;
}

extern int open[MAX_THREADS];
extern int server_shm_size;

#endif