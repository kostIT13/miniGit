#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define strdup _strdup
#endif

// Вспомогательная: создать новый узел
static TreeNode* create_node(const char *name, const char *hash, bool is_file) {
    TreeNode *node = (TreeNode*)calloc(1, sizeof(TreeNode));
    if (!node) return NULL;
    
    node->name = strdup(name);
    node->hash = hash ? strdup(hash) : NULL;
    node->is_file = is_file;
    node->children = NULL;
    node->children_count = 0;
    node->ref_count = 1;
    
    return node;
}

// Клонирование узла (неглубокое - для structural sharing)
TreeNode* tree_clone_node(TreeNode *node) {
    if (!node) return NULL;
    
    TreeNode *clone = (TreeNode*)malloc(sizeof(TreeNode));
    if (!clone) return NULL;
    
    clone->name = node->name ? strdup(node->name) : NULL;
    clone->hash = node->hash ? strdup(node->hash) : NULL;
    clone->is_file = node->is_file;
    clone->children = node->children;  // <-- КЛЮЧЕВОЙ МОМЕНТ: делим детей!
    clone->children_count = node->children_count;
    clone->ref_count = 1;
    
    // Увеличиваем ref_count у общих детей
    for (int i = 0; i < node->children_count; i++) {
        if (node->children[i].ref_count < 1000) { // Защита от переполнения
            node->children[i].ref_count++;
        }
    }
    
    return clone;
}

// Создать пустое дерево
TreeNode* tree_create(void) {
    return create_node("", NULL, false);
}

// Найти дочерний узел по имени
static TreeNode* find_child(TreeNode *tree, const char *name) {
    if (!tree || !tree->children) return NULL;
    
    for (int i = 0; i < tree->children_count; i++) {
        if (strcmp(tree->children[i].name, name) == 0) {
            return &tree->children[i];
        }
    }
    return NULL;
}

// Добавить файл в дерево (рекурсивно)
static TreeNode* add_file_recursive(TreeNode *old_tree, const char **path_parts, int depth) {
    if (!path_parts[depth]) return old_tree;
    
    // Создаем НОВЫЙ узел (неизменяемость!)
    TreeNode *new_tree = tree_clone_node(old_tree);
    if (!new_tree) return NULL;
    
    const char *current_name = path_parts[depth];
    bool is_last = (path_parts[depth + 1] == NULL);
    
    // Ищем существующий дочерний узел
    TreeNode *existing = find_child(new_tree, current_name);
    
    if (is_last) {
        // Это конечный файл - обновляем или создаем
        if (existing) {
            // Обновляем существующий
            for (int i = 0; i < new_tree->children_count; i++) {
                if (strcmp(new_tree->children[i].name, current_name) == 0) {
                    free(new_tree->children[i].hash);
                    new_tree->children[i].hash = strdup(path_parts[0]);
                    new_tree->children[i].is_file = true;
                    break;
                }
            }
        } else {
            // Добавляем новый файл
            new_tree->children_count++;
            new_tree->children = (TreeNode*)realloc(new_tree->children, 
                                                    new_tree->children_count * sizeof(TreeNode));
            TreeNode *new_child = create_node(current_name, path_parts[0], true);
            new_tree->children[new_tree->children_count - 1] = *new_child;
            free(new_child);
        }
    } else {
        // Это папка - рекурсивно спускаемся
        if (existing && !existing->is_file) {
            TreeNode *new_subtree = add_file_recursive(existing, path_parts, depth + 1);
            if (new_subtree) {
                // Обновляем хеш поддерева (упрощенно)
                for (int i = 0; i < new_tree->children_count; i++) {
                    if (strcmp(new_tree->children[i].name, current_name) == 0) {
                        break;
                    }
                }
            }
        } else {
            // Создаем новую папку
            new_tree->children_count++;
            new_tree->children = (TreeNode*)realloc(new_tree->children, 
                                                    new_tree->children_count * sizeof(TreeNode));
            TreeNode *new_subtree = add_file_recursive(tree_create(), path_parts, depth + 1);
            TreeNode *new_child = create_node(current_name, NULL, false);
            new_child->children = new_subtree->children;
            new_child->children_count = new_subtree->children_count;
            new_tree->children[new_tree->children_count - 1] = *new_child;
            free(new_subtree);
        }
    }
    
    return new_tree;
}

// Добавить файл в дерево (публичный API)
TreeNode* tree_add_file(TreeNode *old_tree, const char *path, const char *hash) {
    if (!path || !hash) return NULL;
    
    // Разбиваем путь на части
    char *path_copy = strdup(path);
    if (!path_copy) return NULL;
    
    char *parts[100];
    int count = 0;
    
    char *token = strtok(path_copy, "/\\");
    while (token && count < 100) {
        parts[count++] = token;
        token = strtok(NULL, "/\\");
    }
    
    // Добавляем хеш в начало массива (для передачи в рекурсию)
    const char *args[102];
    args[0] = hash;
    for (int i = 0; i < count; i++) {
        args[i + 1] = parts[i];
    }
    args[count + 1] = NULL;
    
    TreeNode *new_tree = add_file_recursive(old_tree ? old_tree : tree_create(), 
                                            args, 1);
    
    free(path_copy);
    return new_tree;
}

// Получить хеш файла
const char* tree_get_file_hash(TreeNode *tree, const char *path) {
    if (!tree || !path) return NULL;
    
    char *path_copy = strdup(path);
    if (!path_copy) return NULL;
    
    char *token = strtok(path_copy, "/\\");
    TreeNode *current = tree;
    
    while (token) {
        TreeNode *child = find_child(current, token);
        if (!child) {
            free(path_copy);
            return NULL;
        }
        
        char *next = strtok(NULL, "/\\");
        if (!next) {
            // Последний элемент
            free(path_copy);
            return child->is_file ? child->hash : NULL;
        }
        
        current = child;
        token = next;
    }
    
    free(path_copy);
    return NULL;
}

// Проверить существование файла
bool tree_file_exists(TreeNode *tree, const char *path) {
    return tree_get_file_hash(tree, path) != NULL;
}

// Вывести файлы дерева
void tree_print_files(TreeNode *tree, int indent) {
    if (!tree || !tree->children) return;
    
    for (int i = 0; i < tree->children_count; i++) {
        TreeNode *child = &tree->children[i];
        
        for (int j = 0; j < indent; j++) printf("  ");
        printf("%s %s", child->is_file ? "[F]" : "[D]", child->name);
        if (child->hash) printf(" (%s)", child->hash);
        printf("\n");
        
        if (!child->is_file && child->children) {
            tree_print_files(child, indent + 1);
        }
    }
}

// Освободить память
void tree_free(TreeNode *tree) {
    if (!tree) return;
    
    tree->ref_count--;
    if (tree->ref_count > 0) {
        return; // Еще есть ссылки на этот узел
    }
    
    free(tree->name);
    free(tree->hash);
    
    if (tree->children) {
        for (int i = 0; i < tree->children_count; i++) {
            tree_free(&tree->children[i]);
        }
        free(tree->children);
    }
    
    free(tree);
}

// Удалить файл из дерева (возвращает НОВОЕ дерево)
TreeNode* tree_remove_file(TreeNode *old_tree, const char *path) {
    if (!old_tree || !path) return NULL;
    
    // Если файла нет, возвращаем то же дерево
    if (!tree_file_exists(old_tree, path)) {
        return old_tree;
    }
    
    // Упрощенная реализация: создаем клон дерева
    // В полной реализации нужно рекурсивно удалять узел по пути
    TreeNode *new_tree = tree_clone_node(old_tree);
    
    // TODO: Реализовать рекурсивное удаление узла по пути
    // Для учебных целей пока возвращаем клон
    
    return new_tree;
}