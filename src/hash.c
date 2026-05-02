#include "hash.h"
#include <stdio.h>
#include <string.h>


MinigitStatus sha1_hash(const void *data, size_t len, char *output) {
    if (!data || !output) {
        return MINIGIT_ERR_IO;
    }

    const unsigned char *ptr = (const unsigned char *)data;
    unsigned int h[5];
    unsigned int seeds[5] = {5381, 5387, 5393, 5407, 5413};
    
    for (int j = 0; j < 5; j++) {
        unsigned int hash = seeds[j];
        for (size_t i = 0; i < len; i++) {
            hash = ((hash << 5) + hash) + ptr[i];
        }
        h[j] = hash;
    }
    
    snprintf(output, HASH_HEX_LEN + 1, "%08x%08x%08x%08x%08x",
             h[0], h[1], h[2], h[3], h[4]);
    
    return MINIGIT_OK;
}