#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "tinyfile_service.h"

int open[NUM_THREADS] = {1, 1, 1, 1};


/**
This method is used for combining the incoming buffer chunks together. 

Args:
 - result: buffer to hold the result as it grows
 - buffer: the buffer being appended
 - full_length: represents the full length of the input, needed for initializing result if result == NULL
 */
char* append_chunks(char *result, const char *buffer, int full_length) {
    if (result == NULL) {
        result = malloc(full_length + 1);
        if (result == NULL) {
            fprintf(stderr, "malloc failed");
            exit(1);
        }
        strcpy(result, buffer);
    } else {
        strcat(result, buffer);
    }
    return result;
}

void* segment_function(void *arg) {
    segment_t* data = (segment_t*) arg;
    int recv_id = data->seg_id * 9;
    int msg_length, shm_id;
    message_t msg;
    msg.msg_type = 1000;
    msg.destination_id = data->seg_id;
    while (1) {
        printf("Segment %d is looping.\n", data->seg_id);

        if (msgrcv(data->msg_id, &msg, sizeof(message_t), recv_id, 0) == -1) {
            perror("msgrcv failed on thread");
            exit(1);
        }
        msg.msg_type = data->seg_id;
        msg_length = msg.full_msg_length;

        printf("Thread %d received: %s\n", data->seg_id, msg.msg_text);
        open[(data->seg_id) - 1] = 0;

        shm_id = shmget(data->shm_key, sizeof(shared_memory_chunk_t), 0666 | IPC_CREAT | IPC_PRIVATE);
      
        if (shm_id == -1) {
            perror("shmget failed on thread");
            exit(1);
        }

        shared_memory_chunk_t* shm_ptr = (shared_memory_chunk_t*) shmat(shm_id, NULL, 0);
        if (shm_ptr == (shared_memory_chunk_t*) -1) {
            perror("shmat failed on thread");
            exit(1);
        }
        msg.shm_id = shm_id;
        if (msgsnd(data->msg_id, &msg, sizeof(message_t), 0) == -1) {
            perror("msgsnd failed on thread");
            exit(1);
        }

        int finish = 0;
        char *result = NULL;
        while (!finish) {
            if (msgrcv(data->msg_id, &msg, sizeof(message_t), recv_id, 0) == -1) {
                perror("msgrcv failed, chunk receiver, on thread");
                exit(1);
            }
            msg.msg_type = data->seg_id;
            result = append_chunks(result, shm_ptr->chunk_content, msg_length);
            // printf("%s\n", result);
            if (shm_ptr->is_final_chunk) {
                finish = 1;
            }
            if (msgsnd(data->msg_id, &msg, sizeof(message_t), 0) == -1) {
                perror("msgsnd failed, chunk receiver, on thread");
                exit(1);
            }
        }

        // printf("Data written to shared memory: %s\n", result);

        strcpy(shm_ptr->chunk_content, "here");

        if (msgsnd(data->msg_id, &msg, sizeof(message_t), 0) == -1) {
            perror("msgsnd failed on thread");
            exit(1);
        }
        if (shmdt(shm_ptr) == -1) {
            perror("shmdt failed on thread");
            exit(1);
        }
      
        struct shmid_ds buf;
        if (shmctl(shm_id, IPC_RMID, &buf) == -1) {
            perror("shmctl IPC_RMID failed");
            return NULL;
        }
        usleep(5000000);
        open[data->seg_id - 1] = 1;
        usleep(10000);
    }
    return NULL;
}

int main() {
    int msg_id;
    message_t msg_client;
    message_t msg_segment;
    pthread_t segments[NUM_THREADS];
    segment_t segment_data[NUM_THREADS];

    // * The message queue is initialized
    key_t key = ftok("my_message_queue_key", 65);
    msg_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_id == -1) {
        perror("msgget failed");
        exit(1);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        key_t shm_key = ftok("my_shared_memory_key", 75 + i);
        segment_data[i].msg_id = msg_id;
        segment_data[i].seg_id = i + 1;
        segment_data[i].shm_key = shm_key;

        if (pthread_create(&segments[i], NULL, segment_function, &segment_data[i]) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
    }

    printf("TinyFile service is now active!\n");

    // * Develop function for assigning segment to client!
    while (1) {
        // printf("Main thread looping!\n");
        if (msgrcv(msg_id, &msg_client, sizeof(message_t), 100, 0) == -1) {
            perror("msgrcv failed, main thread");
            exit(1);
        }
        msg_client.msg_type = 256;

        printf("Server received: %s\n", msg_client.msg_text);
        int found = 0;
        while (!found) {
            for (int j = 0; j < NUM_THREADS; j++) {
                printf("%d\n", j);
                if (open[j]) {
                    found = 1;
                    msg_client.destination_id = j + 1;
                    break;
                }
            }
        }

        if (msgsnd(msg_id, &msg_client, sizeof(message_t), 0) == -1) {
            perror("msgsnd failed, main thead");
            exit(1);
        }
        usleep(10000);

    }

    return 0;
}