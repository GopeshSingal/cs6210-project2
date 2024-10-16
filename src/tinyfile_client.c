#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include "tinyfile_client.h"
#include "tinyfile_service.h"
#include "tinyfile_library.h"

char * read_file(char* input_path) {
    FILE *input_file = fopen(input_path, "rb");
    if (!input_file) {
        fprintf(stderr, "Error: Unable to open input file: %s\n", input_path);
        return NULL;
    }

    // Get the file size
    fseek(input_file, 0L, SEEK_END);
    size_t input_size = ftell(input_file);
    rewind(input_file);

    // Allocate memory to read the input file
    char *input_data = (char *)malloc(input_size);
    if (!input_data) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(input_file);
        return NULL;
    }

    // Read the input file into the buffer
    size_t read_size = fread(input_data, 1, input_size, input_file);
    fclose(input_file);

    if (read_size != input_size) {
        fprintf(stderr, "Error: Failed to read input file.\n");
        free(input_data);
        return NULL;
    }
    return input_data;
}

int files_in_dir(const char* path, char *files[]) {
    struct dirent* entry;
    DIR* dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "Error: Could not open directory '%s'\n", path);
        return 0;
    }
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        files[count] = malloc(strlen(entry->d_name) + 1 + strlen(path));
        if (files[count] != NULL) {
            strcpy(files[count], path);
            strcat(files[count], entry->d_name);
            count++;
        }
    }
    closedir(dir);
    return count;
}

void* async_function(void* arg) {
    async_thread_t* data = (async_thread_t*) arg;
    int msg_id, shm_id, seg_id, recv_id;
    
    // * Read file from unique pathname
    printf("Thread %d is active!\n", data->thread_id);
    char* buffer = read_file(data->pathname);
    if (!buffer) {
        perror("File read failure");
        exit(1);
    }

    // * Initialize chunks
    int chunk_size = SHM_SIZE - 1;
    int num_chunks = ((strlen(buffer) + chunk_size - 1) / chunk_size);
    char** chunk_buffers = (char**) malloc(num_chunks * sizeof(char*));
    for (int i = 0; i < num_chunks; i++) {
        chunk_buffers[i] = (char*) malloc(chunk_size);
    }
    chunk_input_buffer(buffer, strlen(buffer), chunk_size, chunk_buffers);

    // * Initialize key
    key_t key = ftok("my_message_queue_key", 65);
    msg_id = msgget(key, 0666);
    if (msg_id == -1) {
        perror("msgget failed");
        exit(1);
    }

    // * Initialize message, then send and receive with server
    message_t msg;
    msg.msg_type = 100;
    msg.full_msg_length = strlen(buffer);
    msg.shm_id = 0;
    strcpy(msg.msg_text, "This string is from client");
    if (msgsnd(msg_id, &msg, sizeof(message_t) - sizeof(long), 0) == -1) {
        perror("msgsnd to server failed");
        exit(1);
    }
    if (msgrcv(msg_id, &msg, sizeof(message_t) - sizeof(long), 256, 0) == -1) {
        perror("msgrcv failed");
        exit(1);
    }

    seg_id = msg.destination_id;
    recv_id = seg_id * 9;
    msg.msg_type = recv_id;
    if (msgsnd(msg_id, &msg, sizeof(message_t) - sizeof(long), 0) == -1) {
        perror("msgsnd to thread failed");
        exit(1);
    }

    // * Receive the shared memory access
    if (msgrcv(msg_id, &msg, sizeof(message_t) - sizeof(long), seg_id, 0) == -1) { 
        perror("msgrcv failed, 1");
        exit(1);
    }
    msg.msg_type = recv_id;
    shm_id = msg.shm_id;
    shared_memory_chunk_t* shm_ptr = (shared_memory_chunk_t*) shmat(shm_id, NULL, 0);
    if (shm_ptr == (shared_memory_chunk_t*) -1) {
        perror("shmat failed on thread");
        exit(1);
    }
    printf("Client received %d!\n", shm_id);

    // * Send each chunk one-by-one
    shm_ptr->is_final_chunk = 0;
    for (int i = 0; i < num_chunks; i++) {
        if (i == num_chunks - 1) {
            shm_ptr->is_final_chunk = 1;
        } else {
            shm_ptr->is_final_chunk = 0;
        }
        strcpy(shm_ptr->chunk_content, chunk_buffers[i]);
        // * Send notice that the shared memory is ready
        if (msgsnd(msg_id, &msg, sizeof(message_t) - sizeof(long), 0) == -1) {
            perror("msgsnd failed, chunk sender");
            exit(1);
        }
        // * Receive notice that the shared memory is ready (mainly used for synchronization between client and server)
        if (msgrcv(msg_id, &msg, sizeof(message_t) - sizeof(long), seg_id, 0) == -1) {
            perror("msgrcv failed, chunk sender");
            exit(1);
        }
        msg.msg_type = recv_id;
    }

    if (msgrcv(msg_id, &msg, sizeof(message_t) - sizeof(long), seg_id, 0) == -1) {
        perror("msgrcv failed");
        exit(1);
    }
    printf("Data written to shared memory: %s\n", shm_ptr->chunk_content);
    msg.msg_type = recv_id;
    // * Cleanup
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt failed, 1");
        exit(1);
    }
    for (int i = 0; i < num_chunks; i++) {
        free(chunk_buffers[i]);
    }
    free(chunk_buffers);
    printf("Thread %d is finished!\n", data->thread_id);
}

int main(int argc, char *argv[]) {
    int num_threads;
    char *files[MAX_REQUESTS];
    int file_count = 0;
    int mode = 0;
    pthread_t threads[MAX_REQUESTS];
    async_thread_t thread_data[MAX_REQUESTS];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s SYNC|ASYNC <folder_path>\n", argv[0]);
        return 1;
    }

    const char* files_path = argv[2];
    file_count = files_in_dir(files_path, files);
    printf("File count: %d\n", file_count);
    if (file_count == 0) {
        fprintf(stderr, "No files found in directory");
        return 1;
    }

    if (strcmp(argv[1], "SYNC") == 0) {
        num_threads = 1;
        mode = 0;
    } else if (strcmp(argv[1], "ASYNC") == 0) {
        num_threads = file_count;
        mode = 1;
    } else {
        fprintf(stderr, "Error: First argument must be either 'SYNC' or 'ASYNC'.\n");
        return 1;
    }

    if (mode) {
        for (int i = 0; i < file_count; i++) {
            thread_data[i].thread_id = i;
            strcpy(thread_data[i].pathname, files[i]);
            if (pthread_create(&threads[i], NULL, async_function, &thread_data[i]) != 0) {
                perror("pthread creation failed");
                exit(1);
            }
            usleep(10000);
        }
        for (int i = 0; i < file_count; i++) {
            (void) pthread_join(threads[i], NULL);
            free(files[i]);
        }
    } else {
        for (int i = 0; i < file_count; i++) {
            pthread_t runthread;
            async_thread_t thread;
            thread.thread_id = 1;
            strcpy(thread.pathname, files[i]);
            if (pthread_create(&runthread, NULL, async_function, &thread) != 0) {
                perror("pthread creation failed");
                exit(1);
            }
            (void) pthread_join(runthread, NULL);
        }
        usleep(500000);
        for (int i = 0; i < file_count; i++) {
            free(files[i]);
        }
    }
    return 0;
}