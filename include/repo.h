#ifndef REPO_H
#define REPO_H

#include "minigit.h"
#include "commit.h"

MinigitStatus init_repo_disk(void);

int repo_exists(void);

/* Branch management */
MinigitStatus create_branch(const char *branch_name, Commit *commit);
Commit* get_branch_head(const char *branch_name);
MinigitStatus checkout(Commit *commit);

#endif