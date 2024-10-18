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

struct llnode* add_llnode(struct llnode * tail, int id) {
    // printf("CREATING NEW TASK\n");
    struct llnode* new_node = (struct llnode*)(malloc(sizeof(struct llnode)));
    new_node->recv_id = id;
    new_node->next = NULL;
    if (tail) {
        // printf("moving tail for %d and new tail is %d\n", tail, new_node);
        tail->next = new_node;
    }
    return new_node;
}

int remove_llnode(struct llnode * head) {
    int return_id;
    if (!head) {
        return -1;
    }
    return_id = head->recv_id;
    if (!head->next) {
        printf("free2\n");
        free(head);
    } else {
        struct llnode* tmp = head;
        head = head->next;
        printf("free3\n");
        free(tmp);
    }
    return return_id;
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

struct qnode* create_node(int id) {
    printf("create node: %d\n", id);
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

int fake_dequeue(struct client_queue* queue) {
    int return_id;
    printf("queue size: %d\n", queue->size);
    // if (queue->size == 0) {
    //     return -1;
    // }
    // if (queue->front == queue->rear) {
    //     // hash_map_delete(queue->front->recv_id);
    //     printf("REMOVE1\n");
    //     return_id = remove_llnode(queue->front->head);
    //     if (!queue->front->head) {
    //         printf("free1\n");
    //         free(queue->front);
    //         queue->size--;
    //         queue->front = queue->rear = NULL;
    //     }
    // } else {
        // printf("REMOVE2\n");
        // return_id = remove_llnode(queue->front->head);
        struct qnode* tmp = queue->front;
        queue->front = queue->front->next;
        queue->rear = tmp;
        printf("front %d and rear %d\n", queue->front, queue->rear);
        // if (!queue->front->head) {
        //     // hash_map_delete(queue->front->recv_id);
        //     queue->rear->next = queue->front;
        //     printf("Freeing %d\n", tmp);
        //     free(tmp);
        //     queue->size--;
        // } else {
        //     queue->rear = tmp;
        // }
    // }
    return 0;
}

typedef struct HashNode {
    int key;
    struct qnode* value;
    struct HashNode* next;
} HashNode;
typedef struct HashMap {
    HashNode* buckets[HASH_MAP_SIZE];
} HashMap;

uint32_t fnv1a_hash(uint32_t data) {
    uint32_t hash = FNV_32_OFFSET_BASIS;

    for (int i = 0; i < 4; i++) {
        hash ^= (data & 0xFF);
        hash *= FNV_32_PRIME;
        data >>= 8;            
    }

    return hash % HASH_MAP_SIZE;
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
    // printf("putting value: %d\n", value);
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
            printf("returning qnode %d\n", current->value);
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

    printf("Key %d not found in the HashMap.\n", key);
}

void free_hash_map(HashMap* map) {
    for (int i = 0; i < HASH_MAP_SIZE; i++) {
        HashNode* current = map->buckets[i];
        while (current != NULL) {
            HashNode* next = current->next;
            free(current);
            current = next;
        }
    }
    free(map);
}

void hash_map_display(HashMap* map) {
    for (int i = 0; i < HASH_MAP_SIZE; i++) {
        HashNode* current = map->buckets[i];
        printf("Bucket %d: ", i);
        while (current != NULL) {
            printf("(%d, %d) -> ", current->key, current->value);
            current = current->next;
        }
        printf("NULL\n");
    }
}

extern int open[MAX_THREADS];
extern int server_shm_size;

#endif