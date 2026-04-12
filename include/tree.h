#ifndef TREE_H
#define TREE_H

#include "minigit.h"
#include <stdbool.h>

// Узел дерева (файл или папка)
typedef struct TreeNode {
    char *name;             // Имя файла или папки
    char *hash;             // Хеш содержимого (для файла) или поддерева
    bool is_file;           // true = файл (blob), false = папка (tree)
    struct TreeNode *children; // Массив дочерних узлов (для папки)
    int children_count;     // Количество дочерних узлов
    int ref_count;          // Счетчик ссылок для structural sharing
} TreeNode;

// Создать пустое дерево
TreeNode* tree_create(void);

// Добавить/обновить файл в дереве (возвращает НОВОЕ дерево)
TreeNode* tree_add_file(TreeNode *old_tree, const char *path, const char *hash);

// Удалить файл из дерева (возвращает НОВОЕ дерево)
TreeNode* tree_remove_file(TreeNode *old_tree, const char *path);

// Получить хеш файла по пути
const char* tree_get_file_hash(TreeNode *tree, const char *path);

// Проверить существование файла
bool tree_file_exists(TreeNode *tree, const char *path);

// Вывести список файлов
void tree_print_files(TreeNode *tree, int indent);

// Освободить память (с учетом ref_count)
void tree_free(TreeNode *tree);

// Клонировать узел (неглубокое копирование для structural sharing)
TreeNode* tree_clone_node(TreeNode *node);

#endif