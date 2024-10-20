#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include "tinyfile_service.h"


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

int main() {
    int shm_id, msg_id, msg_length;

    // * The message queue is initialized
    key_t key = ftok("my_message_queue_key", 65);
    msg_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_id == -1) {
        perror("msgget failed");
        exit(1);
    }
    printf("TinyFile service is now active!\n");
    while (1) {
        printf("Looping\n");
        // * Search the message queue for an available client
        message_t msg;
        if (msgrcv(msg_id, &msg, sizeof(message_t), 1, 0) == -1) {          //! msg receive
            perror("msgrcv failed, 1");
            exit(1);
        }
        msg.msg_type = 2;
        msg_length = msg.full_msg_length;

        printf("Server received: %s\n", msg.msg_text);
        // * Initialize the shared memory for the message and return it to the client
        shm_id = shmget(key, sizeof(shared_memory_chunk_t), 0666 | IPC_CREAT);
        if (shm_id == -1) {
            perror("shmget failed, 1");
            exit(1);
        }
        shared_memory_chunk_t *shm_ptr = (shared_memory_chunk_t *) shmat(shm_id, NULL, 0);
        if (shm_ptr == (shared_memory_chunk_t*) -1) {
            perror("shmat failed, 1");
            exit(1);
        }
        msg.shm_id = shm_id;
        if (msgsnd(msg_id, &msg, sizeof(message_t), 0) == -1) {             //! msg send
            perror("msgsnd failed, 1");
            exit(1);
        }

        // * Prepare for the chunk receiving
        int final = 0;
        int j = 0;
        char *result = NULL;
        // * Until the shared memory flag for "is_final_chunk" is active
        // * we repeatedly receive chunks
        while (!final) {
            // * Receive notice that shared memory is ready
            if (msgrcv(msg_id, &msg, sizeof(message_t), 1, 0) == -1) {      //! msg recv
                perror("msgrcv failed, chunk receiver");
                exit(1);
            }
            msg.msg_type = 2;
            result = append_chunks(result, shm_ptr->chunk_content, msg_length);
            printf("Running result: %s\n", result);
            if (shm_ptr->is_final_chunk) {
                final = 1;
            }
            // * Return notice that shared memory is ready
            if (msgsnd(msg_id, &msg, sizeof(message_t), 0) == -1) {         //! msg send
                perror("msgsnd failed, chunk receiver");
                exit(1);
            }
            j++;
        }
        printf("Data written to shared memory: %s\n", result);
        strcpy(shm_ptr->chunk_content, "Here");
        if (msgsnd(msg_id, &msg, sizeof(message_t), 0) == -1 ) {
            perror("msgsnd failed, 2");
            exit(1);
        }
        if (shmdt(shm_ptr) == -1) {
            perror("shmdt failed, 1");
            exit(1);
        }
        usleep(10000);
    }
    return 0;
}