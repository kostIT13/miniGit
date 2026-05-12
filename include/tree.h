#ifndef TREE_H
#define TREE_H

#include "minigit.h"
#include <stdbool.h>

typedef struct TreeNode {
    char *name;                      // Имя файла или папки
    char *hash;                      // Хэш содержимого (для файла)
    bool is_file;                    // true = файл, false = папка
    struct TreeNode *children;       // Массив дочерних узлов
    int children_count;              // Количество детей
    int ref_count;                   // Счётчик ссылок (structural sharing)
} TreeNode;

TreeNode* tree_create(void);

TreeNode* tree_add_file(TreeNode *old_tree, const char *path, const char *hash);

TreeNode* tree_remove_file(TreeNode *old_tree, const char *path);

const char* tree_get_file_hash(TreeNode *tree, const char *path);

bool tree_file_exists(TreeNode *tree, const char *path);

void tree_print_files(TreeNode *tree, int indent);

void tree_free(TreeNode *tree);

TreeNode* tree_clone_node(TreeNode *node);

#endif