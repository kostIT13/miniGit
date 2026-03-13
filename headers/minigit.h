#ifndef MINIGIT_H
#define MINIGIT_H

#include <stdint.h>
#include <time.h>

#define HASH_HEX_LEN 40
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