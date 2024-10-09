#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../snappy-c/snappy.h"

#define MAX_FILE_SIZE 10485760 // 10 MB (adjust based on your needs)

void compress_file(const char *input_path, const char *output_path) {
    // Open the input file for reading
    FILE *input_file = fopen(input_path, "rb");
    if (!input_file) {
        fprintf(stderr, "Error: Unable to open input file: %s\n", input_path);
        return;
    }

    // Get the file size
    fseek(input_file, 0L, SEEK_END);
    size_t input_size = ftell(input_file);
    rewind(input_file);

    if (input_size > MAX_FILE_SIZE) {
        fprintf(stderr, "Error: Input file size exceeds maximum allowed size.\n");
        fclose(input_file);
        return;
    }

    // Allocate memory to read the input file
    char *input_data = (char *)malloc(input_size);
    if (!input_data) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(input_file);
        return;
    }

    // Read the input file into the buffer
    size_t read_size = fread(input_data, 1, input_size, input_file);
    fclose(input_file);

    if (read_size != input_size) {
        fprintf(stderr, "Error: Failed to read input file.\n");
        free(input_data);
        return;
    }

    // Prepare the buffer for compressed data
    size_t compressed_max_size = snappy_max_compressed_length(input_size);
    // char *compressed_data = (char *)malloc(compressed_max_size);
    // if (!compressed_data) {
    //     fprintf(stderr, "Error: Memory allocation for compressed data failed.\n");
    //     free(input_data);
    //     return;
    // }

    // Compress the data using Snappy
    // size_t compressed_size;
    // struct snappy_env *a;
    // int env_init_status = snappy_init_env(a);
    
    // if (env_init_status) {
    //     fprintf(stderr, "Error: Snappy init env failed.\n");
    //     free(input_data);
    //     free(compressed_data);
    //     return;
    // }

    // int status = snappy_compress(a, input_data, input_size, compressed_data, &compressed_size);

    // if (status != 0) {
    //     fprintf(stderr, "Error: Snappy compression failed.\n");
    //     free(input_data);
    //     free(compressed_data);
    //     return;
    // }

    // // Open the output file for writing the compressed data
    // FILE *output_file = fopen(output_path, "wb");
    // if (!output_file) {
    //     fprintf(stderr, "Error: Unable to open output file: %s\n", output_path);
    //     free(input_data);
    //     free(compressed_data);
    //     return;
    // }

    // // Write the compressed data to the output file
    // fwrite(compressed_data, 1, compressed_size, output_file);
    // fclose(output_file);

    // Free allocated memory
    free(input_data);
    // free(compressed_data);

    printf("File compressed successfully: %s\n", output_path);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file_path> <output_file_path>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];

    compress_file(input_path, output_path);

    return 0;
}
