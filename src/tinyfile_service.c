#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <getopt.h>
#include "tinyfile_service.h"
#include "tinyfile_library.h"

int open[MAX_THREADS] = {1, 1, 1, 1, 1, 1, 1, 1};
int server_shm_size = SHM_SIZE;

void* segment_function(void *arg) {
    segment_t* data = (segment_t*) arg;
    int recv_id = data->seg_id * 9;
    int msg_length, shm_id;
    int complete = 0;
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.destination_id = data->seg_id;
    printf("Segment %d is created!\n", data->seg_id);
    while (1) {
        if (msgrcv(data->msg_id, &msg, sizeof(message_t) - sizeof(long), recv_id, 0) == -1) {
            perror("msgrcv failed on thread");
            exit(1);
        }
        printf("Segment %d is active! Working on: %s\n", data->seg_id, msg.msg_text);
        if (msg.msg_text[0] ==  '\0' || msg.msg_text == NULL) {
            printf("Segment %d is skipping invalid input!\n", data->seg_id);
            open[data->seg_id - 1] = 1;
            continue;
        }
        msg_length = msg.full_msg_length;
        memset(&msg, 0, sizeof(msg));
        msg.mtype = data->seg_id;

        shm_id = shmget(IPC_PRIVATE, sizeof(shared_memory_chunk_t), 0666 | IPC_CREAT);
        if (shm_id == -1) {
            perror("shmget failed on thread");
            exit(1);
        }

        shared_memory_chunk_t* shm_ptr = (shared_memory_chunk_t*) shmat(shm_id, NULL, 0);
        if (shm_ptr == (shared_memory_chunk_t*) -1) {
            perror("shmat failed on thread SERVER");
            exit(1);
        }
        msg.shm_id = shm_id;
        if (msgsnd(data->msg_id, &msg, sizeof(message_t) - sizeof(long), 0) == -1) {
            perror("msgsnd failed on thread SERVER");
            exit(1);
        }

        int finish = 0;
        char *result = NULL;
        while (!finish) {
            if (msgrcv(data->msg_id, &msg, sizeof(message_t) - sizeof(long), recv_id, 0) == -1) {
                perror("msgrcv failed, chunk receiver, on thread");
                exit(1);
            }
            memset(&msg, 0, sizeof(msg));
            msg.mtype = data->seg_id;
            result = append_chunks(result, shm_ptr->chunk_content, msg_length, server_shm_size);
            // printf("%s\n", result);
            if (shm_ptr->is_final_chunk) {
                finish = 1;
            }
            if (msgsnd(data->msg_id, &msg, sizeof(message_t) - sizeof(long), 0) == -1) {
                perror("msgsnd failed, chunk receiver, on thread");
                exit(1);
            }
        }

        size_t compressed_size;
        char * compressed_data = compress_file(result, &compressed_size);
        free(result);

        // * Initialize chunks
        int chunk_size = SHM_SIZE;
        int num_chunks = ((compressed_size + chunk_size - 1) / chunk_size);
        char** chunk_buffers = (char**) malloc(num_chunks * sizeof(char*));
        for (int i = 0; i < num_chunks; i++) {
            chunk_buffers[i] = (char*) malloc(chunk_size);
        }
        chunk_input_buffer(compressed_data, compressed_size, chunk_size, chunk_buffers);
        free(compressed_data);

        strcpy(shm_ptr->chunk_content, "here");

        if (msgsnd(data->msg_id, &msg, sizeof(message_t) - sizeof(long), 0) == -1) {
            perror("msgsnd failed on thread second fail");
            exit(1);
        }
        if (shmdt(shm_ptr) == -1) {
            perror("shmdt failed on thread");
            exit(1);
        }
      
        if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
            perror("shmctl IPC_RMID failed");
            printf("failed to kill shm_id: %d, segment: %d\n", shm_id, data->seg_id);
            exit(1);
        }
        open[data->seg_id - 1] = 1;
        complete++;
        printf("Segment %d has finished task #%d\n", data->seg_id, complete);
        usleep(10000);
    }
    return NULL;
}

/*
    HOW TO RUN:
    ./tinyfile_service.o --n_sms #segments --sms_size #bytes_per_segment
*/
int main(int argc, char* argv[]) {
    int n_sms = 0;
    int sms_size = 0;

    static struct option long_options[] = {
        {"n_sms",    required_argument, 0,  'n' }, 
        {"sms_size", required_argument, 0,  's' },
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "n:s:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'n':
                n_sms = atoi(optarg);
                break;
            case 's':
                sms_size = atoi(optarg); 
                break;
            case '?': 
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    if (n_sms <= 0) {
        printf("No --n_sms provided; Using --n_sms = 4");
        n_sms = 4;
    } else if (n_sms > 8) {
        printf("Exceed maximum segments; Using --n_sms = 8");
        n_sms = MAX_THREADS;
    }

    if (sms_size <= 0) {
        printf("No --sms_size provided; Using --sms_size = 1024");
        sms_size = 1024;
    }
    server_shm_size = sms_size;

    int msg_id;
    message_t msg_client;
    message_t msg_segment;
    pthread_t segments[n_sms];
    segment_t segment_data[n_sms];

    // * The message queue is initialized
    key_t key = ftok("my_message_queue_key", 65);
    message_t msgc;
    msg_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_id == -1) {
        perror("msgget failed");
        exit(1);
    }

    while (msgrcv(msg_id, &msgc, sizeof(message_t) - sizeof(long), 0, IPC_NOWAIT) != -1) {
        printf("Cleared extra message!\n");
    }
    
    for (int i = 0; i < n_sms; i++) {
        segment_data[i].msg_id = msg_id;
        segment_data[i].seg_id = i + 1;

        if (pthread_create(&segments[i], NULL, segment_function, &segment_data[i]) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
    }

    printf("TinyFile service is now active!\n");

    // * Develop function for assigning segment to client!
    while (1) {
        // printf("Main thread looping!\n");
        if (msgrcv(msg_id, &msg_client, sizeof(message_t) - sizeof(long), SERVER_ACCESS_MTYPE, 0) == -1) {
            perror("msgrcv failed, main thread");
            exit(1);
        }
        msg_client.mtype = SERVER_MTYPE;
        msg_client.shm_size = server_shm_size;

        // printf("Server received: %s\n", msg_client.msg_text);
        int found = 0;
        while (!found) {
            for (int j = 0; j < n_sms; j++) {
                if (open[j]) {
                    found = 1;
                    msg_client.destination_id = j + 1;
                    open[j] = 0;
                    break;
                }
            }
        }

        if (msgsnd(msg_id, &msg_client, sizeof(message_t) - sizeof(long), 0) == -1) {
            perror("msgsnd failed, main thead");
            exit(1);
        }
        usleep(10000);

    }

    return 0;
}