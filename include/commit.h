#ifndef COMMIT_H
#define COMMIT_H

#include "minigit.h"
#include "tree.h"
#include <time.h>
#include <stdbool.h>

typedef struct Commit {
    char hash[HASH_HEX_LEN + 1];        // Хэш этого коммита (40 символов + \0)
    char parent_hash[HASH_HEX_LEN + 1]; // Хэш родительского коммита
    char tree_hash[HASH_HEX_LEN + 1];   // Хэш корневого дерева файлов
    TreeNode *tree;                     // Указатель на дерево в памяти
    char message[1024];                 // Сообщение коммита
    char author[64];                    // Автор (например, "miniGit")
    time_t timestamp;                   // Время создания (Unix timestamp)
} Commit;

Commit* init_repo(void);

Commit* add_file(Commit *old_commit, const char *path, const char *content);

Commit* remove_file(Commit *old_commit, const char *path);

Commit* commit(Commit *state, const char *message);

char* get_file_content(Commit *commit, const char *path, size_t *out_len);

bool get_file_exists(Commit *commit, const char *path);

void print_commit(Commit *commit);

void print_history(Commit *commit);

void print_files(Commit *commit);

Commit* commit_create(TreeNode *tree, const char *message, const char *parent_hash);

Commit* commit_load(const char *hash);

MinigitStatus commit_save(Commit *commit);

void commit_free(Commit *commit);

char* get_head_commit(void);

MinigitStatus update_head(const char *commit_hash);

#endif