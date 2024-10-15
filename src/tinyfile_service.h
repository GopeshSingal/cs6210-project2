#ifndef __TINYFILE_SERVICE_H
#define __TINYFILE_SERVICE_H

#include <sys/_types/_key_t.h>
#define SHM_KEY 1234
#define SHM_SIZE 6
#define MSG_KEY 5678
#define MSG_SIZE 128
#define NUM_THREADS 4

typedef struct shared_memory_chunk {
    char chunk_content[SHM_SIZE];
    int chunk_size;
    int is_final_chunk; // 0 if no, 1 if yes
} shared_memory_chunk_t;

typedef struct message {
    long msg_type;
    char msg_text[MSG_SIZE];
    int shm_id;
    int full_msg_length;
    int destination_id;
} message_t;

typedef struct segment {
    int msg_id;
    int seg_id;
    key_t shm_key;
} segment_t;

extern int open[NUM_THREADS];

#endif