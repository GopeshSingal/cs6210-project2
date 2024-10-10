#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include "tinyfile_service.h"

int main() {
    int shm_id, msg_id;
    key_t key = ftok("my_message_queue_key", 65);
    msg_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_id == -1) {
        perror("msgget failed");
        exit(1);
    }
    printf("TinyFile service is now active!\n");
    while (1) {
        message_t msg;
        if (msgrcv(msg_id, &msg, sizeof(message_t), 1, 0) == -1) {
            perror("msgrcv failed, 1");
            exit(1);
        }
        msg.msg_type = 2;
        printf("Server received: %s\n", msg.msg_text);
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
        if (msgsnd(msg_id, &msg, sizeof(message_t), 0) == -1) {
            perror("msgsnd failed, 1");
            exit(1);
        }
        if (msgrcv(msg_id, &msg, sizeof(message_t), 1, 0) == -1) {
            perror("msgrcv failed, 2");
            exit(1);
        }
        msg.msg_type = 2;
        printf("Data written to shared memory: %s\n", shm_ptr->chunk_content);
        strcpy(shm_ptr->chunk_content, "The server has taken the data from the shared memory!");
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