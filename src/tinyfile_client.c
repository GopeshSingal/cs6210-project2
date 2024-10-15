#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tinyfile_service.h"
#include "tinyfile_library.h"


int main() {
    // * Initialization
    int msg_id, shm_id;
    int chunk_size = SHM_SIZE - 1;
    char buffer[] = "This is a test message for chunking!";
    int num_chunks = ((strlen(buffer) + chunk_size - 1) / chunk_size);
    char **chunk_buffers = (char **) malloc(num_chunks * sizeof(char*));
    chunk_input_buffer(buffer, strlen(buffer), chunk_size, chunk_buffers);
    key_t key = ftok("my_message_queue_key", 65);
    msg_id = msgget(key, 0666);
    if (msg_id == -1) {
        perror("msgget failed, 1");
        exit(1);
    }
    message_t msg;
    msg.msg_type = 1;
    msg.shm_id = 0;
    strcpy(msg.msg_text, "This string is from the client!");

    // * Send a message to the server to notify existence
    printf("Sending a message!\n");
    if (msgsnd(msg_id, &msg, sizeof(msg.msg_text), 0) == -1) {      //! msg send
        perror("msgsnd failed, 1");
        exit(1);
    }
    printf("Client sent message\n");

    // * Receive the shared memory key from the server
    if (msgrcv(msg_id, &msg, sizeof(message_t), 2, 0) == -1) {      //! msg receive
        perror("msgrcv failed, 1");
        exit(1);
    }
    msg.msg_type = 1;
    shm_id = msg.shm_id;
    printf("Client received %d!\n", shm_id);

    shared_memory_chunk_t *shm_ptr = (shared_memory_chunk_t *) shmat(shm_id, NULL, 0);
    if (shm_ptr == (shared_memory_chunk_t*) -1) {
        perror("shmat failed, 1");
        exit(1);
    }

    // * Iterate through each of the chunks to give to the server
    shm_ptr->is_final_chunk = 0;
    for (int i = 0; i < num_chunks; i++) {
        if (i == num_chunks - 1) {
            shm_ptr->is_final_chunk = 1;
        } else {
            shm_ptr->is_final_chunk = 0;
        }
        strcpy(shm_ptr->chunk_content, chunk_buffers[i]);
        // * Send notice that the shared memory is ready
        if (msgsnd(msg_id, &msg, sizeof(message_t), 0) == -1) {
            perror("msgsnd failed, chunk sender");
            exit(1);
        }
        // * Receive notice that the shared memory is ready (mainly used for synchronization between client and server)
        if (msgrcv(msg_id, &msg, sizeof(message_t), 2, 0) == -1) {
            perror("msgrcv failed, chunk sender");
            exit(1);
        }
        msg.msg_type = 1;
    }

    // * Wait to receive notification from server that shared memory is safe to write
    if (msgrcv(msg_id, &msg, sizeof(message_t), 2, 0) == -1) {
        perror("msgrcv failed, 2");
        exit(1);
    }
    printf("Data written to shared memory: %s\n", shm_ptr->chunk_content);
    msg.msg_type = 1;

    // * Cleanup
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt failed, 1");
        exit(1);
    }
    for (int i = 0; i < num_chunks; i++) {
        free(chunk_buffers[i]);
    }
    free(chunk_buffers);
    return 0;
}