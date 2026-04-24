/* tree.c — реализация дерева файлов для miniGit */
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define strdup _strdup
#endif

/* ======================================================================
   Вспомогательные функции
   ====================================================================== */

// Создать новый узел (файл или папку)
static TreeNode* create_node(const char *name, const char *hash, bool is_file) {
    TreeNode *node = (TreeNode*)calloc(1, sizeof(TreeNode));
    if (!node) return NULL;
    
    node->name = name ? strdup(name) : NULL;
    node->hash = hash ? strdup(hash) : NULL;
    node->is_file = is_file;
    node->children = NULL;
    node->children_count = 0;
    node->ref_count = 1;
    
    return node;
}

// Клонировать узел с НОВЫМ массивом детей (для безопасной модификации)
// Это ключ к структурному разделению: копируем только путь, остальное — по ссылке
static TreeNode* clone_with_fresh_children(TreeNode *src) {
    if (!src) return NULL;
    
    TreeNode *dst = (TreeNode*)calloc(1, sizeof(TreeNode));
    if (!dst) return NULL;
    
    dst->name = src->name ? strdup(src->name) : NULL;
    dst->hash = src->hash ? strdup(src->hash) : NULL;
    dst->is_file = src->is_file;
    dst->ref_count = 1;
    
    // 🔑 Создаём НОВЫЙ массив детей и копируем в него записи
    if (src->children_count > 0) {
        dst->children_count = src->children_count;
        dst->children = (TreeNode*)calloc(src->children_count, sizeof(TreeNode));
        if (!dst->children) {
            free(dst->name);
            free(dst->hash);
            free(dst);
            return NULL;
        }
        for (int i = 0; i < src->children_count; i++) {
            dst->children[i] = src->children[i];  // Копируем структуру узла
            // Увеличиваем ref_count у общего ребёнка
            if (dst->children[i].ref_count < 10000) {
                dst->children[i].ref_count++;
            }
        }
    }
    return dst;
}

// Найти индекс ребёнка по имени
static int find_child_index(TreeNode *tree, const char *name) {
    if (!tree || !name || !tree->children) return -1;
    for (int i = 0; i < tree->children_count; i++) {
        if (tree->children[i].name && strcmp(tree->children[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* ======================================================================
   Публичные функции создания/удаления
   ====================================================================== */

TreeNode* tree_create(void) {
    return create_node("", NULL, false);
}

/* ======================================================================
   Добавление файла (рекурсивно, с персистентностью)
   ====================================================================== */
static TreeNode* add_recursive(TreeNode *old_node, char **parts, int depth, const char *final_hash) {
    if (!old_node || !parts[depth]) return old_node;
    
    const char *name = parts[depth];
    bool is_last = (parts[depth + 1] == NULL);
    
    // Создаём новый узел с собственным массивом детей
    TreeNode *new_node = clone_with_fresh_children(old_node);
    if (!new_node) return NULL;
    
    if (is_last) {
        // 🔹 Добавляем/обновляем файл
        int idx = find_child_index(new_node, name);
        if (idx >= 0) {
            // Обновляем существующий файл
            free(new_node->children[idx].hash);
            new_node->children[idx].hash = strdup(final_hash);
            new_node->children[idx].is_file = true;
        } else {
            // Добавляем новый файл
            new_node->children_count++;
            new_node->children = realloc(new_node->children, 
                                         new_node->children_count * sizeof(TreeNode));
            if (!new_node->children) {
                tree_free(new_node);
                return NULL;
            }
            TreeNode *entry = &new_node->children[new_node->children_count - 1];
            entry->name = strdup(name);
            entry->hash = strdup(final_hash);
            entry->is_file = true;
            entry->children = NULL;
            entry->children_count = 0;
            entry->ref_count = 1;
        }
        return new_node;
    } else {
        // 🔹 Рекурсивно спускаемся по директории
        int idx = find_child_index(new_node, name);
        if (idx >= 0 && !new_node->children[idx].is_file) {
            TreeNode *new_subtree = add_recursive(&new_node->children[idx], parts, depth + 1, final_hash);
            if (new_subtree && new_subtree != &new_node->children[idx]) {
                new_node->children[idx] = *new_subtree;
            }
        } else {
            // Создаём новую директорию в пути
            new_node->children_count++;
            new_node->children = realloc(new_node->children, 
                                         new_node->children_count * sizeof(TreeNode));
            if (!new_node->children) {
                tree_free(new_node);
                return NULL;
            }
            TreeNode *entry = &new_node->children[new_node->children_count - 1];
            entry->name = strdup(name);
            entry->hash = NULL;
            entry->is_file = false;
            entry->children = NULL;
            entry->children_count = 0;
            entry->ref_count = 1;
            
            // Рекурсивно создаём остальную часть пути
            TreeNode *subtree = add_recursive(tree_create(), parts, depth + 1, final_hash);
            if (subtree) {
                entry->children = subtree->children;
                entry->children_count = subtree->children_count;
                free(subtree);  // Освобождаем обёртку, дети уже скопированы
            }
        }
        return new_node;
    }
}

TreeNode* tree_add_file(TreeNode *old_tree, const char *path, const char *hash) {
    if (!old_tree || !path || !hash) return NULL;
    
    // Парсим путь на компоненты
    char *path_copy = strdup(path);
    if (!path_copy) return NULL;
    
    char *parts[100] = {0};
    int count = 0;
    char *token = strtok(path_copy, "/\\");
    while (token && count < 99) {
        parts[count++] = token;
        token = strtok(NULL, "/\\");
    }
    parts[count] = NULL;
    
    TreeNode *result = add_recursive(old_tree, parts, 0, hash);
    
    free(path_copy);
    return result;
}

/* ======================================================================
   Удаление файла (рекурсивно, с персистентностью)
   ====================================================================== */
static TreeNode* remove_recursive(TreeNode *old_tree, char **parts, int depth) {
    if (!old_tree || !parts[depth]) return old_tree;
    
    const char *name = parts[depth];
    bool is_last = (parts[depth + 1] == NULL);
    
    int idx = find_child_index(old_tree, name);
    if (idx == -1) return old_tree;  // Файл не найден
    
    if (is_last) {
        // 🔹 Удаляем этот файл: создаём новое дерево без этого ребёнка
        TreeNode *new_tree = clone_with_fresh_children(old_tree);
        if (!new_tree) return NULL;
        
        if (old_tree->children_count == 1) {
            // Удаляем единственный ребёнок
            new_tree->children = NULL;
            new_tree->children_count = 0;
        } else {
            // Копируем всех, кроме удаляемого
            new_tree->children_count = old_tree->children_count - 1;
            new_tree->children = (TreeNode*)calloc(new_tree->children_count, sizeof(TreeNode));
            
            int j = 0;
            for (int i = 0; i < old_tree->children_count; i++) {
                if (i == idx) continue;  // Пропускаем удаляемый
                new_tree->children[j] = old_tree->children[i];
                if (new_tree->children[j].ref_count < 10000) {
                    new_tree->children[j].ref_count++;
                }
                j++;
            }
        }
        return new_tree;
    } else {
        // 🔹 Рекурсивно удаляем в подкаталоге
        TreeNode *new_subtree = remove_recursive(&old_tree->children[idx], parts, depth + 1);
        
        // Если поддерево не изменилось — возвращаем оригинал
        if (new_subtree == &old_tree->children[idx]) {
            return old_tree;
        }
        
        // Поддерево изменилось — клонируем текущий узел
        TreeNode *new_tree = clone_with_fresh_children(old_tree);
        if (!new_tree) return NULL;
        
        new_tree->children_count = old_tree->children_count;
        new_tree->children = (TreeNode*)calloc(new_tree->children_count, sizeof(TreeNode));
        
        for (int i = 0; i < old_tree->children_count; i++) {
            if (i == idx) {
                new_tree->children[i] = *new_subtree;
            } else {
                new_tree->children[i] = old_tree->children[i];
                if (new_tree->children[i].ref_count < 10000) {
                    new_tree->children[i].ref_count++;
                }
            }
        }
        return new_tree;
    }
}

TreeNode* tree_remove_file(TreeNode *old_tree, const char *path) {
    if (!old_tree || !path) return NULL;
    if (!tree_file_exists(old_tree, path)) return old_tree;
    
    char *path_copy = strdup(path);
    if (!path_copy) return NULL;
    
    char *parts[100] = {0};
    int count = 0;
    char *token = strtok(path_copy, "/\\");
    while (token && count < 99) {
        parts[count++] = token;
        token = strtok(NULL, "/\\");
    }
    parts[count] = NULL;
    
    TreeNode *result = remove_recursive(old_tree, parts, 0);
    
    free(path_copy);
    return result;
}

/* ======================================================================
   Поиск и вывод
   ====================================================================== */

const char* tree_get_file_hash(TreeNode *tree, const char *path) {
    if (!tree || !path) return NULL;
    
    char *path_copy = strdup(path);
    if (!path_copy) return NULL;
    
    char *token = strtok(path_copy, "/\\");
    TreeNode *current = tree;
    
    while (token) {
        TreeNode *child = NULL;
        for (int i = 0; i < current->children_count; i++) {
            if (current->children[i].name && strcmp(current->children[i].name, token) == 0) {
                child = &current->children[i];
                break;
            }
        }
        if (!child) {
            free(path_copy);
            return NULL;
        }
        
        char *next = strtok(NULL, "/\\");
        if (!next) {
            free(path_copy);
            return child->is_file ? child->hash : NULL;
        }
        current = child;
        token = next;
    }
    
    free(path_copy);
    return NULL;
}

bool tree_file_exists(TreeNode *tree, const char *path) {
    return tree_get_file_hash(tree, path) != NULL;
}

void tree_print_files(TreeNode *tree, int indent) {
    if (!tree || !tree->children) return;
    
    for (int i = 0; i < tree->children_count; i++) {
        TreeNode *child = &tree->children[i];
        for (int j = 0; j < indent; j++) printf("  ");
        printf("%s %s", child->is_file ? "[F]" : "[D]", child->name ? child->name : "(null)");
        if (child->hash) printf(" (%s)", child->hash);
        printf("\n");
        if (!child->is_file && child->children) {
            tree_print_files(child, indent + 1);
        }
    }
}

/* ======================================================================
   Освобождение памяти (с учётом ref_count)
   ====================================================================== */
void tree_free(TreeNode *tree) {
    if (!tree) return;
    
    tree->ref_count--;
    if (tree->ref_count > 0) {
        return;  // Есть другие ссылки — не освобождаем
    }
    
    // Освобождаем строки
    free(tree->name);
    free(tree->hash);
    
    // 🔧 Освобождаем МАССИВ детей (если он был выделен отдельно)
    // Но НЕ рекурсивно освобождаем сами узлы-дети — они общие, их ref_count управляет жизнью
    if (tree->children) {
        free(tree->children);
    }
    
    free(tree);
}