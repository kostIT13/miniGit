#include "hash.h"
#include <stdio.h>
#include <string.h>


MinigitStatus sha1_hash(const void *data, size_t len, char *output) {
    if (!data || !output) {
        return MINIGIT_ERR_IO;
    }

    unsigned int hash = 5381;
    const unsigned char *ptr = (const unsigned char *)data;
    
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + ptr[i];
    }

    snprintf(output, HASH_HEX_LEN + 1, "%08x", hash);
    
    return MINIGIT_OK;
}