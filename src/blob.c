#include "blob.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#endif


void get_object_path(const char *hash, char *path_output, size_t path_size) {
    snprintf(path_output, path_size, "%s/%s/%c%c/%s", 
             MINIGIT_DIR, OBJECTS_DIR, hash[0], hash[1], hash + 2);
}

MinigitStatus blob_save(const void *content, size_t len, char *hash_output) {
    if (!content || !hash_output) {
        return MINIGIT_ERR_IO;
    }

    
    MinigitStatus status = sha1_hash(content, len, hash_output);
    if (status != MINIGIT_OK) {
        return status;
    }

    char path[512];
    get_object_path(hash_output, path, sizeof(path));

    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s/%s/%c%c", 
             MINIGIT_DIR, OBJECTS_DIR, hash_output[0], hash_output[1]);
    mkdir(dir_path, 0755);

    FILE *f = fopen(path, "wb");
    if (!f) {
        return MINIGIT_ERR_IO;
    }
    
    fwrite(content, 1, len, f);
    fclose(f);

    return MINIGIT_OK;
}

char* blob_load(const char *hash, size_t *out_len) {
    if (!hash) {
        return NULL;
    }

    char path[512];
    get_object_path(hash, path, sizeof(path));

    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (out_len) {
        *out_len = (size_t)size;
    }

    char *content = (char*)malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    fread(content, 1, size, f);
    content[size] = '\0'; 
    fclose(f);

    return content;
}