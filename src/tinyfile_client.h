#ifndef __TINYFILE_CLIENT_H
#define __TINYFILE_CLIENT_H

#define MAX_REQUESTS 512

typedef struct async_thread {
    char pathname[50];
    int thread_id;
    pid_t client_id;
} async_thread_t;

#endif