#ifndef HASH_H
#define HASH_H

#include "minigit.h"

MinigitStatus content_hash(const void *data, size_t len, char *output);

#endif