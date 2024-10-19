#ifndef __TINYFILE_LIBRARY_H
#define __TINYFILE_LIBRARY_H

#define SERVER_MTYPE 256
#define SERVER_ACCESS_MTYPE 100
#define QUEUE_MTYPE 512
#define MAX_SHM_SIZE 8192
#define SHM_SIZE 1024
#define MSG_SIZE 128

typedef struct shared_memory_chunk {
    char chunk_content[MAX_SHM_SIZE + 1];
    int chunk_size;
    int is_final_chunk; // 0 if no, 1 if yes
} shared_memory_chunk_t;

typedef struct message {
    long mtype;
    char msg_text[MSG_SIZE];
    int shm_size;
    int shm_id;
    int full_msg_length;
    int destination_id;
    int count;
    pid_t client_id;
} message_t;

extern void my_copy_str(char *dest, char *src, size_t num);
extern void chunk_input_buffer(const char *buffer, size_t buffer_size, size_t chunk_size, char **chunk_buffers);
extern char * compress_file(const char *input_data, size_t * compressed_size);
extern char* append_compressed_chunks(char *result, const char *buffer, int full_length, int chunk_size, int offset);
extern char* append_chunks(char *result, const char *buffer, int full_length, int chunk_size);
#endif