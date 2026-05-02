#ifndef BLOB_H
#define BLOB_H

#include "minigit.h"

MinigitStatus blob_save(const void *content, size_t len, char *hash_output);

char* blob_load(const char *hash, size_t *out_len);

void get_object_path(const char *hash, char *path_output, size_t path_size);

#endif