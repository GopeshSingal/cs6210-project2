extern void chunk_input_buffer(const char *buffer, size_t buffer_size, size_t chunk_size, char **chunk_buffers);
extern char * compress_file(const char *input_data, size_t * compressed_size);
extern void uncompress_buffer(char * compressed_data, size_t compressed_size);