#ifndef __TINYFILE_SERVICE_H
#define __TINYFILE_SERVICE_H

#include <sys/_types/_key_t.h>
#define NUM_THREADS 5
typedef struct segment {
    int msg_id;
    int seg_id;
    key_t shm_key;
} segment_t;

extern int open[NUM_THREADS];

#endif