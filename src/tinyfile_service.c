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
HashMap * q_map;

void* segment_function(void *arg) {
    segment_t* data = (segment_t*) arg;
    int recv_id = data->seg_id + 9;
    int msg_length, shm_id;
    int complete = 0;
    int client_id = 0;
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.destination_id = data->seg_id;
    printf("Segment %d is created!\n", data->seg_id);
    while (1) {
        if (msgrcv(data->msg_id, &msg, sizeof(message_t) - sizeof(long), recv_id, 0) == -1) {
            perror("msgrcv failed on thread");
            exit(1);
        }
        client_id = msg.client_id;
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
        // Send compressed length back to client
        msg.full_msg_length = compressed_size;
        strcpy(shm_ptr->chunk_content, "here");
        if (msgsnd(data->msg_id, &msg, sizeof(message_t) - sizeof(long), 0) == -1) {
            perror("msgsnd failed on thread second fail");
            exit(1);
        }

        // * Send each chunk one-by-one
        shm_ptr->is_final_chunk = 0;
        for (int i = 0; i < num_chunks; i++) {
            if (i == num_chunks - 1) {
                shm_ptr->is_final_chunk = 1;
            } else {
                shm_ptr->is_final_chunk = 0;
            }
            my_copy_str(shm_ptr->chunk_content, chunk_buffers[i], server_shm_size);
            // * Send notice that the shared memory is ready
            if (msgsnd(data->msg_id, &msg, sizeof(message_t) - sizeof(long), 0) == -1) {
                perror("msgsnd failed, chunk sender server");
                exit(1);
            }
            free(chunk_buffers[i]);
            // * Receive notice that the shared memory is ready (mainly used for synchronization between client and server)
            if (msgrcv(data->msg_id, &msg, sizeof(message_t) - sizeof(long), recv_id, 0) == -1) {
                perror("msgrcv failed, chunk sender server");
                exit(1);
            }
            memset(&msg, 0, sizeof(msg));
            msg.mtype = data->seg_id;
        }
        free(chunk_buffers);

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

void* queue_function(void *arg) {
    queue_thread_data_t* data = (queue_thread_data_t*) arg;
    message_t msg;
    while (1) {
        if (msgrcv(data->msg_id, &msg, sizeof(message_t) - sizeof(long), QUEUE_MTYPE, 0) == -1) {
            perror("msgrcv failed, queue thread");
            exit(1);
        }
        enqueue(data->queue, msg.destination_id);
        struct qnode* curr_qnode = hash_map_get(q_map, msg.client_id);
        if (!curr_qnode) {
            curr_qnode = fake_enqueue(data->fake_queue, msg.client_id);
            hash_map_put(q_map, msg.client_id, curr_qnode);
        }
        add_llnode(curr_qnode, msg.destination_id);
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
    q_map = create_hash_map();
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
            perror("segment_thread creation failed\n");
            exit(1);
        }
    }

    struct client_queue* queue = create_queue();
    struct client_queue* fake_queue = create_queue();
    pthread_t queue_thread;
    queue_thread_data_t queue_data;
    queue_data.msg_id = msg_id;
    queue_data.queue = queue; // set up circular queue of client threads
    queue_data.fake_queue = fake_queue;
    if (pthread_create(&queue_thread, NULL, queue_function, &queue_data) != 0) {
        perror("queue_thread creation failed\n");
        exit(1);
    }

    printf("TinyFile service is now active!\n");

    // * Develop function for assigning segment to client!
    while (1) {
        while (1) {
            if (fake_queue->front != NULL && fake_queue->front->head != NULL) {
                break;
            }
        }
        int fake_recv_id = fake_dequeue(fake_queue, q_map);

        msg_client.mtype = fake_recv_id;
        msg_client.shm_size = server_shm_size;

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