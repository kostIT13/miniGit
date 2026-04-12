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