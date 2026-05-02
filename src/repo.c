#define _CRT_SECURE_NO_WARNINGS

#include "repo.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#endif

static int create_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }
    return mkdir(path, 0755);
}

static int create_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "%s", content);
    fclose(f);
    return 0;
}

int repo_exists(void) {
    struct stat st;
    char path[512];
    snprintf(path, sizeof(path), "%s", MINIGIT_DIR);
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

MinigitStatus init_repo_disk(void) {
    char path[512];

    if (repo_exists()) {
        fprintf(stderr, "Error: Repository already exists in this directory.\n");
        return MINIGIT_ERR_EXISTS;
    }

    if (create_directory(MINIGIT_DIR) != 0) {
        fprintf(stderr, "Error: Cannot create %s directory: %s\n", MINIGIT_DIR, strerror(errno));
        return MINIGIT_ERR_IO;
    }

    snprintf(path, sizeof(path), "%s/%s", MINIGIT_DIR, OBJECTS_DIR);
    if (create_directory(path) != 0) {
        fprintf(stderr, "Error: Cannot create objects directory\n");
        return MINIGIT_ERR_IO;
    }

    snprintf(path, sizeof(path), "%s/%s", MINIGIT_DIR, REFS_DIR);
    if (create_directory(path) != 0) {
        fprintf(stderr, "Error: Cannot create refs directory\n");
        return MINIGIT_ERR_IO;
    }

    snprintf(path, sizeof(path), "%s/%s", MINIGIT_DIR, HEAD_FILE);
    if (create_file(path, "ref: refs/heads/master\n") != 0) {
        fprintf(stderr, "Error: Cannot create HEAD file\n");
        return MINIGIT_ERR_IO;
    }

    printf("Initialized empty miniGit repository in %s/\n", MINIGIT_DIR);
    return MINIGIT_OK;
}

/* Helper: get path to ref file */
static void get_ref_path(char *buf, size_t buf_size, const char *branch_name) {
    snprintf(buf, buf_size, "%s/%s/heads/%s", MINIGIT_DIR, REFS_DIR, branch_name);
}

/* Helper: read hash from ref file */
static MinigitStatus read_ref_hash(const char *branch_name, char *hash_out) {
    char path[512];
    get_ref_path(path, sizeof(path), branch_name);
    
    FILE *f = fopen(path, "r");
    if (!f) {
        return MINIGIT_ERR_IO;
    }
    
    if (fgets(hash_out, HASH_HEX_LEN + 1, f) == NULL) {
        fclose(f);
        return MINIGIT_ERR_IO;
    }
    fclose(f);
    
    /* Ensure null termination */
    hash_out[HASH_HEX_LEN] = '\0';
    return MINIGIT_OK;
}

/* Helper: write hash to ref file */
static MinigitStatus write_ref_hash(const char *branch_name, const char *hash) {
    char path[512];
    get_ref_path(path, sizeof(path), branch_name);
    
    FILE *f = fopen(path, "w");
    if (!f) {
        return MINIGIT_ERR_IO;
    }
    
    fprintf(f, "%s", hash);
    fclose(f);
    return MINIGIT_OK;
}

/* Create a new branch pointing to given commit */
MinigitStatus create_branch(const char *branch_name, Commit *commit) {
    if (!branch_name || !commit) {
        return MINIGIT_ERR_IO;
    }
    
    /* Check if branch already exists */
    char dummy[HASH_HEX_LEN + 1];
    if (read_ref_hash(branch_name, dummy) == MINIGIT_OK) {
        fprintf(stderr, "Error: Branch '%s' already exists.\n", branch_name);
        return MINIGIT_ERR_EXISTS;
    }
    
    /* Write commit hash to ref file */
    MinigitStatus status = write_ref_hash(branch_name, commit->hash);
    if (status != MINIGIT_OK) {
        fprintf(stderr, "Error: Cannot create branch '%s'.\n", branch_name);
        return status;
    }
    
    printf("Branch '%s' created at commit %s\n", branch_name, commit->hash);
    return MINIGIT_OK;
}

/* Get the head commit of a branch */
Commit* get_branch_head(const char *branch_name) {
    if (!branch_name) {
        return NULL;
    }
    
    char hash[HASH_HEX_LEN + 1];
    if (read_ref_hash(branch_name, hash) != MINIGIT_OK) {
        fprintf(stderr, "Error: Branch '%s' does not exist.\n", branch_name);
        return NULL;
    }
    
    /* Load commit from disk using hash */
    Commit *commit = commit_load(hash);
    if (!commit) {
        fprintf(stderr, "Error: Cannot load commit %s for branch '%s'.\n", hash, branch_name);
    }
    
    return commit;
}

/* Checkout a commit (simplified: just return the commit pointer) */
MinigitStatus checkout(Commit *commit) {
    if (!commit) {
        return MINIGIT_ERR_IO;
    }
    
    /* In a real implementation, we would update HEAD and working directory.
     * For simplicity, we just print a message and return success.
     */
    printf("Checked out commit %s\n", commit->hash);
    
    /* Optionally update HEAD file to point to this commit directly (detached HEAD) */
    char head_path[512];
    snprintf(head_path, sizeof(head_path), "%s/%s", MINIGIT_DIR, HEAD_FILE);
    FILE *f = fopen(head_path, "w");
    if (f) {
        fprintf(f, "%s\n", commit->hash);
        fclose(f);
    }
    
    return MINIGIT_OK;
}