#ifndef __TINYFILE_SERVICE_H
#define __TINYFILE_SERVICE_H

#define NUM_THREADS 5
typedef struct segment {
    int msg_id;
    int seg_id;
} segment_t;

extern int open[NUM_THREADS];

#endif