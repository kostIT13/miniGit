#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
    #define strdup _strdup
#endif

#include "commit.h"
#include "blob.h"
#include "hash.h"
#include "repo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Форматирование содержимого коммита для хеширования
static char* format_commit_content(Commit *commit) {
    char *content = (char*)malloc(2048);
    if (!content) return NULL;
    
    snprintf(content, 2048, 
             "tree %s\n"
             "parent %s\n"
             "author %s\n"
             "committer %s\n"
             "%ld\n"
             "\n"
             "%s",
             commit->tree_hash,
             commit->parent_hash,
             commit->author,
             commit->author,
             (long)commit->timestamp,
             commit->message);
    
    return content;
}

// Упрощённое хеширование дерева (для совместимости)
static char* save_tree_to_blob(TreeNode *tree) {
    if (!tree) return NULL;
    
    char *buffer = (char*)malloc(4096);
    if (!buffer) return NULL;
    
    buffer[0] = '\0';
    snprintf(buffer, 4096, "tree_data_%p", (void*)tree);
    
    char *hash = (char*)malloc(HASH_HEX_LEN + 1);
    if (!hash) {
        free(buffer);
        return NULL;
    }
    
    sha1_hash(buffer, strlen(buffer), hash);
    free(buffer);
    
    return hash;
}


// Сохранить структуру дерева в отдельный файл
static MinigitStatus save_tree_structure(TreeNode *tree, const char *commit_hash) {
    if (!tree || !commit_hash) return MINIGIT_ERR_IO;
    
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/tree_%s.dat", MINIGIT_DIR, OBJECTS_DIR, commit_hash);
    
    FILE *f = fopen(path, "w");
    if (!f) return MINIGIT_ERR_IO;
    
    // Рекурсивная запись дерева
    fprintf(f, "%d\n", tree->children_count);
    for (int i = 0; i < tree->children_count; i++) {
        TreeNode *child = &tree->children[i];
        fprintf(f, "%s|%s|%d|%d\n", 
                child->name ? child->name : "",
                child->hash ? child->hash : "",
                child->is_file ? 1 : 0,
                child->children_count);
        
        // Рекурсивно сохраняем поддерево если это папка
        if (!child->is_file && child->children && child->children_count > 0) {
            // Для простоты сохраняем только первый уровень
            // В полной реализации нужна рекурсия
        }
    }
    
    fclose(f);
    return MINIGIT_OK;
}

// Загрузить структуру дерева из файла
static TreeNode* load_tree_structure(const char *commit_hash) {
    if (!commit_hash) return tree_create();
    
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/tree_%s.dat", MINIGIT_DIR, OBJECTS_DIR, commit_hash);
    
    FILE *f = fopen(path, "r");
    if (!f) return tree_create();  // Если файла нет - пустое дерево
    
    TreeNode *tree = tree_create();
    
    int count;
    if (fscanf(f, "%d\n", &count) != 1) {
        fclose(f);
        return tree;
    }
    
    if (count > 0) {
        tree->children = (TreeNode*)calloc(count, sizeof(TreeNode));
        tree->children_count = count;
        
        for (int i = 0; i < count; i++) {
            char name[256] = {0};
            char hash[64] = {0};
            int is_file = 0;
            int sub_count = 0;
            
            if (fscanf(f, "%255[^|]|%63[^|]|%d|%d\n", name, hash, &is_file, &sub_count) >= 3) {
                tree->children[i].name = strlen(name) > 0 ? strdup(name) : NULL;
                tree->children[i].hash = strlen(hash) > 0 ? strdup(hash) : NULL;
                tree->children[i].is_file = (is_file == 1);
                tree->children[i].children = NULL;
                tree->children[i].children_count = 0;
                tree->children[i].ref_count = 1;
            }
        }
    }
    
    fclose(f);
    return tree;
}

// === ФУНКЦИИ ИЗ ТЗ ===

Commit* init_repo(void) {
    if (repo_exists()) {
        char *head_hash = get_head_commit();
        if (head_hash) {
            Commit *commit = commit_load(head_hash);
            free(head_hash);
            return commit;
        }
        fprintf(stderr, "Error: Repository exists but HEAD is corrupted.\n");
        return NULL;
    }
    
    if (init_repo_disk() != MINIGIT_OK) {
        return NULL;
    }
    
    TreeNode *empty_tree = tree_create();
    Commit *commit = commit_create(empty_tree, "Initial empty commit", NULL);
    tree_free(empty_tree);

    if (commit) {
        update_head(commit->hash);
    }
    
    return commit;
}

Commit* add_file(Commit *old_commit, const char *path, const char *content) {
    if (!old_commit || !path || !content) return NULL;
    
    char content_hash[HASH_HEX_LEN + 1];
    if (blob_save(content, strlen(content), content_hash) != MINIGIT_OK) {
        return NULL;
    }
    
    TreeNode *new_tree = tree_add_file(old_commit->tree, path, content_hash);
    if (!new_tree) {
        return NULL;
    }
    
    Commit *new_commit = (Commit*)calloc(1, sizeof(Commit));
    if (!new_commit) {
        tree_free(new_tree);
        return NULL;
    }
    
    strncpy(new_commit->parent_hash, old_commit->hash, HASH_HEX_LEN);
    new_commit->parent_hash[HASH_HEX_LEN] = '\0';
    strncpy(new_commit->author, old_commit->author, sizeof(new_commit->author) - 1);
    new_commit->timestamp = time(NULL);
    
    new_commit->tree = new_tree;
    
    char *tree_hash = save_tree_to_blob(new_tree);
    if (tree_hash) {
        strncpy(new_commit->tree_hash, tree_hash, HASH_HEX_LEN);
        new_commit->tree_hash[HASH_HEX_LEN] = '\0';
        free(tree_hash);
    }
    
    snprintf(new_commit->message, sizeof(new_commit->message), "Add file: %s", path);

    char *content_fmt = format_commit_content(new_commit);
    if (content_fmt) {
        sha1_hash(content_fmt, strlen(content_fmt), new_commit->hash);
        free(content_fmt);
    }
    
    commit_save(new_commit);
    return new_commit;
}

Commit* remove_file(Commit *old_commit, const char *path) {
    if (!old_commit || !path) return NULL;
    
    if (!tree_file_exists(old_commit->tree, path)) {
        return old_commit;
    }
    
    TreeNode *new_tree = tree_remove_file(old_commit->tree, path);
    if (!new_tree) {
        return NULL;
    }
    
    Commit *new_commit = (Commit*)calloc(1, sizeof(Commit));
    if (!new_commit) {
        tree_free(new_tree);
        return NULL;
    }
    
    strncpy(new_commit->parent_hash, old_commit->hash, HASH_HEX_LEN);
    new_commit->parent_hash[HASH_HEX_LEN] = '\0';
    strncpy(new_commit->author, old_commit->author, sizeof(new_commit->author) - 1);
    new_commit->timestamp = time(NULL);
    new_commit->tree = new_tree;
    
    char *tree_hash = save_tree_to_blob(new_tree);
    if (tree_hash) {
        strncpy(new_commit->tree_hash, tree_hash, HASH_HEX_LEN);
        new_commit->tree_hash[HASH_HEX_LEN] = '\0';
        free(tree_hash);
    }
    
    snprintf(new_commit->message, sizeof(new_commit->message), "Remove file: %s", path);
    
    char *content_fmt = format_commit_content(new_commit);
    if (content_fmt) {
        sha1_hash(content_fmt, strlen(content_fmt), new_commit->hash);
        free(content_fmt);
    }
    
    commit_save(new_commit);
    return new_commit;
}

Commit* commit(Commit *state, const char *message) {
    if (!state || !message) return NULL;
    
    if (strlen(state->message) > 0) {
        commit_save(state);
        return state;
    }
    
    strncpy(state->message, message, sizeof(state->message) - 1);
    state->message[sizeof(state->message) - 1] = '\0';
    
    char *content = format_commit_content(state);
    if (content) {
        sha1_hash(content, strlen(content), state->hash);
        free(content);
    }
    
    commit_save(state);
    update_head(state->hash);
    
    return state;
}

char* get_file_content(Commit *commit, const char *path, size_t *out_len) {
    if (!commit || !path) return NULL;
    
    const char *file_hash = tree_get_file_hash(commit->tree, path);
    if (!file_hash) {
        return NULL;
    }
    
    return blob_load(file_hash, out_len);
}

bool get_file_exists(Commit *commit, const char *path) {
    if (!commit || !path) return false;
    return tree_file_exists(commit->tree, path);
}

void print_commit(Commit *commit) {
    if (!commit) return;
    
    printf("commit %s\n", commit->hash);
    if (commit->parent_hash[0] != '\0') {
        printf("parent %s\n", commit->parent_hash);
    }
    printf("Author: %s\n", commit->author);
    printf("Date:   %s", ctime(&commit->timestamp));
    printf("\n    %s\n", commit->message);
    
    printf("\nChanged files:\n");
    tree_print_files(commit->tree, 2);
}

void print_history(Commit *commit) {
    Commit *current = commit;
    
    while (current) {
        print_commit(current);
        printf("\n");
        
        if (current->parent_hash[0] != '\0') {
            Commit *parent = commit_load(current->parent_hash);
            commit_free(current);
            current = parent;
        } else {
            commit_free(current);
            current = NULL;
        }
    }
}

void print_files(Commit *commit) {
    if (!commit || !commit->tree) return;
    
    printf("Files in commit %s:\n", commit->hash);
    tree_print_files(commit->tree, 1);
}


Commit* commit_create(TreeNode *tree, const char *message, const char *parent_hash) {
    if (!tree || !message) return NULL;
    
    Commit *commit = (Commit*)calloc(1, sizeof(Commit));
    if (!commit) return NULL;
    
    char *tree_hash = save_tree_to_blob(tree);
    if (tree_hash) {
        strncpy(commit->tree_hash, tree_hash, HASH_HEX_LEN);
        commit->tree_hash[HASH_HEX_LEN] = '\0';
        free(tree_hash);
    }
    
    if (parent_hash && strlen(parent_hash) > 0) {
        strncpy(commit->parent_hash, parent_hash, HASH_HEX_LEN);
        commit->parent_hash[HASH_HEX_LEN] = '\0';
    }
    
    strncpy(commit->message, message, sizeof(commit->message) - 1);
    commit->message[sizeof(commit->message) - 1] = '\0';
    
    strncpy(commit->author, "miniGit", sizeof(commit->author) - 1);
    commit->timestamp = time(NULL);
    
    commit->tree = tree;
    
    char *content = format_commit_content(commit);
    if (content) {
        sha1_hash(content, strlen(content), commit->hash);
        free(content);
    }
    
    commit_save(commit);
    
    return commit;
}

MinigitStatus commit_save(Commit *commit) {
    if (!commit) return MINIGIT_ERR_IO;
    
    char path[512];
    get_object_path(commit->hash, path, sizeof(path));
    
    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s/%s/%c%c", 
             MINIGIT_DIR, OBJECTS_DIR, commit->hash[0], commit->hash[1]);
    
    #ifdef _WIN32
        _mkdir(dir_path);
    #else
        mkdir(dir_path, 0755);
    #endif
    
    FILE *f = fopen(path, "w");
    if (!f) return MINIGIT_ERR_IO;
    
    fprintf(f, "tree %s\n", commit->tree_hash);
    fprintf(f, "parent %s\n", commit->parent_hash);
    fprintf(f, "author %s\n", commit->author);
    fprintf(f, "committer %s\n", commit->author);
    fprintf(f, "%ld\n", (long)commit->timestamp);
    fprintf(f, "\n");
    fprintf(f, "%s\n", commit->message);
    
    fclose(f);
    
    save_tree_structure(commit->tree, commit->hash);
    
    return MINIGIT_OK;
}

Commit* commit_load(const char *hash) {
    if (!hash) return NULL;
    
    char path[512];
    get_object_path(hash, path, sizeof(path));
    
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    
    Commit *commit = (Commit*)calloc(1, sizeof(Commit));
    if (!commit) {
        fclose(f);
        return NULL;
    }
    
    strncpy(commit->hash, hash, HASH_HEX_LEN);
    commit->hash[HASH_HEX_LEN] = '\0';
    
    char line[1024];
    
    if (fgets(line, sizeof(line), f) && strncmp(line, "tree ", 5) == 0) {
        sscanf(line + 5, "%8s", commit->tree_hash);
    }
    
    if (fgets(line, sizeof(line), f) && strncmp(line, "parent ", 7) == 0) {
        sscanf(line + 7, "%8s", commit->parent_hash);
    }
    
    if (fgets(line, sizeof(line), f) && strncmp(line, "author ", 7) == 0) {
        sscanf(line + 7, "%63[^\n]", commit->author);
    }
    
    fgets(line, sizeof(line), f);
    
    if (fgets(line, sizeof(line), f)) {
        commit->timestamp = (time_t)atol(line);
    }
    
    fgets(line, sizeof(line), f);
    
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        strncpy(commit->message, line, sizeof(commit->message) - 1);
    }
    
    fclose(f);
    
    commit->tree = load_tree_structure(hash);
    
    return commit;
}

void commit_free(Commit *commit) {
    if (commit) {
        if (commit->tree) {
            tree_free(commit->tree);
        }
        free(commit);
    }
}

char* get_head_commit(void) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", MINIGIT_DIR, HEAD_FILE);
    
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    
    char line[256];
    if (fgets(line, sizeof(line), f)) {
        fclose(f);
        
        if (strncmp(line, "ref: ", 5) == 0) {
            char ref_path[512];
            snprintf(ref_path, sizeof(ref_path), "%s/%s", MINIGIT_DIR, line + 5);
            ref_path[strcspn(ref_path, "\n")] = '\0';
            
            f = fopen(ref_path, "r");
            if (!f) return NULL;
            
            if (fgets(line, sizeof(line), f)) {
                fclose(f);
                line[strcspn(line, "\n")] = '\0';
                return strdup(line);
            }
        } else {
            fclose(f);
            line[strcspn(line, "\n")] = '\0';
            return strdup(line);
        }
    }
    
    fclose(f);
    return NULL;
}

MinigitStatus update_head(const char *commit_hash) {
    if (!commit_hash) return MINIGIT_ERR_IO;
    
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/heads/master", MINIGIT_DIR, REFS_DIR);
    
    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s/%s/heads", MINIGIT_DIR, REFS_DIR);
    
    #ifdef _WIN32
        _mkdir(dir_path);
    #else
        mkdir(dir_path, 0755);
    #endif
    
    FILE *f = fopen(path, "w");
    if (!f) return MINIGIT_ERR_IO;
    
    fprintf(f, "%s\n", commit_hash);
    fclose(f);
    
    return MINIGIT_OK;
}