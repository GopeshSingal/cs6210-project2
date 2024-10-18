#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include "tinyfile_library.h"
#include "./snappy-c/snappy.h"

void my_copy_str(char *dest, char *src, size_t num) {
    for(size_t i = 0; i < num; i++) {
        dest[i] = src[i];
    }
}

void chunk_input_buffer(const char *buffer, size_t buffer_size, size_t chunk_size, char **chunk_buffers) {
    size_t offset = 0;
    size_t chunk_index = 0;
    while (offset < buffer_size) {
        size_t current_chunk_size = (buffer_size - offset < chunk_size) ? (buffer_size - offset) : chunk_size;
        chunk_buffers[chunk_index] = (char *)malloc(current_chunk_size);
        // strncpy(chunk_buffers[chunk_index], buffer + offset, current_chunk_size);
        my_copy_str(chunk_buffers[chunk_index], buffer + offset, current_chunk_size);
        offset += current_chunk_size;
        chunk_index++;
    }
    return;
}

char* append_chunks(char *result, const char *buffer, int full_length, int chunk_size) {
    if (result == NULL) {
        if (full_length < chunk_size) {
            result = malloc(full_length);
            strncpy(result, buffer, full_length);
        } else {
            result = malloc(chunk_size);
            strncpy(result, buffer, chunk_size);
        }
        if (result == NULL) {
            fprintf(stderr, "malloc failed");
            exit(1);
        }
    } else {
        result = realloc(result, strlen(result) + chunk_size + 1);
        if (result == NULL) {
            fprintf(stderr, "realloc failed");
            exit(1);
        }
        strcat(result, buffer);
    }
    return result;
}

char* append_compressed_chunks(char *result, const char *buffer, int full_length, int chunk_size, int offset) {
    if (result == NULL) {
        result = malloc(chunk_size);
        if (result == NULL) {
            fprintf(stderr, "malloc failed");
            exit(1);
        }
        my_copy_str(result, buffer, chunk_size);
    } else {
        result = realloc(result, offset + chunk_size + 1);
        if (result == NULL) {
            fprintf(stderr, "realloc failed");
            exit(1);
        }
        my_copy_str(result + offset, buffer, chunk_size);
    }
    return result;
}

char * compress_file(const char *input_data, size_t * compressed_size) {
    // Prepare the buffer for compressed data
    size_t input_size = strlen(input_data);
    size_t compressed_max_size = snappy_max_compressed_length(input_size);
    char *compressed_data = (char *)malloc(compressed_max_size);
    if (!compressed_data) {
        fprintf(stderr, "Error: Memory allocation for compressed data failed.\n");
        return NULL;
    }

    // Compress the data using Snappy
    struct snappy_env a;
    
    if (snappy_init_env(&a)) {
        fprintf(stderr, "Error: Snappy init env failed.\n");
        free(compressed_data);
        return NULL;
    }
    
    int status = snappy_compress(&a, input_data, input_size, compressed_data, compressed_size);
    snappy_free_env(&a);
    // printf("File compressed successfully: \n");
    return compressed_data;
}

/* Function:  uncompress_buffer 
 * --------------------
 *  Uncompresses a buffer of compressed data and writes it to a file
 *
 *  compressed_data: buffer of compressed data
 *  compressed_size: size of the compressed data buffer
 */
void uncompress_buffer(char * compressed_data, size_t compressed_size) {

    // Prepare the buffer for compressed data
    size_t uncompressed_size;
    printf("UNCOMP LEN: %zu\n", compressed_size);
    if (!snappy_uncompressed_length(compressed_data, compressed_size, &uncompressed_size)) {
        fprintf(stderr, "Error: Failed to get uncompressed length.\n");
        return;
    }
    char *uncompressed_data = (char *)malloc(uncompressed_size);
    if (!uncompressed_data) {
        fprintf(stderr, "Error: Memory allocation for compressed data failed.\n");
        return;
    }

    // Compress the data using Snappy
    printf("compressed size: %zu\n", compressed_size);
    int status = snappy_uncompress(compressed_data, compressed_size, uncompressed_data);
    if (status != 0) {
        fprintf(stderr, "Error: Snappy uncompression failed.\n");
        free(uncompressed_data);
        return;
    }

    // Open the output file for writing the compressed data
    FILE *output_file = fopen("../bin/output/uncomp_test", "wb");
    if (!output_file) {
        fprintf(stderr, "Error: Unable to open output file: \n");
        free(uncompressed_data);
        return;
    }

    // Write the compressed data to the output file
    fwrite(uncompressed_data, 1, uncompressed_size, output_file);
    fclose(output_file);

    // Free allocated memory
    free(uncompressed_data);

    printf("File uncompressed successfully:\n");
}