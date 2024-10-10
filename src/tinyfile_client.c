#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tinyfile_service.h"

void chunk_input_buffer(const char *buffer, size_t buffer_size, size_t chunk_size, char **chunk_buffers) {
    size_t offset = 0;
    size_t chunk_index = 0;
    while (offset < buffer_size) {
        size_t current_chunk_size = (buffer_size - offset < chunk_size) ? (buffer_size - offset) : chunk_size;
        chunk_buffers[chunk_index] = (char *)malloc(current_chunk_size + 1); //! Remove '+ 1' when you don't need to print each chunk
        strncpy(chunk_buffers[chunk_index], buffer + offset, current_chunk_size);
        chunk_buffers[chunk_index][current_chunk_size] = '\0'; //! Remove this when you don't need to print each chunk
        offset += current_chunk_size;
        chunk_index++;
    }
    return;
}

int main() {
    int msg_id, shm_id;
    int chunk_size = SHM_SIZE;
    char buffer[] = "This is a test message for chunking!";
    char **chunk_buffers = (char **) malloc(((strlen(buffer) + chunk_size - 1) / chunk_size) * sizeof(char*));
    chunk_input_buffer(buffer, strlen(buffer), chunk_size, chunk_buffers);
    for (size_t i = 0; i < (strlen(buffer) + chunk_size - 1) / chunk_size; i++) {
        printf("Chunk %zu: %s\n", i + 1, chunk_buffers[i]);
    }
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
    if (msgsnd(msg_id, &msg, sizeof(msg.msg_text), 0) == -1) {
        perror("msgsnd failed, 1");
        exit(1);
    }
    printf("Client sent message\n");
    if (msgrcv(msg_id, &msg, sizeof(message_t), 2, 0) == -1) {
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
    strcpy(shm_ptr->chunk_content, "This string is in shared memory from the client!");
    if (msgsnd(msg_id, &msg, sizeof(message_t), 0) == -1) {
        perror("msgsnd failed, 2");
        exit(1);
    }
    if (msgrcv(msg_id, &msg, sizeof(message_t), 2, 0) == -1) {
        perror("msgrcv failed, 2");
        exit(1);
    }
    printf("Data written to shared memory: %s\n", shm_ptr->chunk_content);
    msg.msg_type = 1;
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt failed, 1");
        exit(1);
    }
    printf("I reached this statement somehow!\n");
    return 0;
}