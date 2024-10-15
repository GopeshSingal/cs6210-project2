#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include "tinyfile_library.h"

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