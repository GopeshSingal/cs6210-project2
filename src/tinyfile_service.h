#ifndef __TINYFILE_SERVICE_H
#define __TINYFILE_SERVICE_H

#define NUM_THREADS 5
#define MAX_THREADS 8
typedef struct segment {
    int msg_id;
    int seg_id;
} segment_t;

extern int open[MAX_THREADS];
extern int server_shm_size;

#endif