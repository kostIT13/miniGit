#ifndef MINIGIT_H
#define MINIGIT_H

#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
    #define strdup _strdup
#endif

#include <stdint.h>
#include <time.h>
#include <stddef.h>
#include <stdbool.h>  

#define HASH_HEX_LEN 8
#define MINIGIT_DIR ".minigit"
#define OBJECTS_DIR "objects"
#define REFS_DIR "refs"
#define HEAD_FILE "HEAD"

typedef struct {
    char hash[HASH_HEX_LEN + 1];
} ObjectHash;

typedef enum {
    MINIGIT_OK = 0,
    MINIGIT_ERR_EXISTS = 1,
    MINIGIT_ERR_IO = 2,
    MINIGIT_ERR_MEMORY = 3
} MinigitStatus;

#endif