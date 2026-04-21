#ifndef REPO_H
#define REPO_H

#include "minigit.h"

MinigitStatus init_repo_disk(void);

int repo_exists(void);

#endif