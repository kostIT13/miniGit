#ifndef COMMIT_H
#define COMMIT_H

#include "minigit.h"
#include "tree.h"
#include <time.h>
#include <stdbool.h>

typedef struct {
    char hash[HASH_HEX_LEN + 1];        // Хеш этого коммита
    char parent_hash[HASH_HEX_LEN + 1]; // Хеш родителя (пустой для первого)
    char tree_hash[HASH_HEX_LEN + 1];   // Хеш дерева файлов
    TreeNode *tree;                     // Указатель на дерево в памяти
    char message[1024];                 // Сообщение коммита
    char author[64];                    // Автор
    time_t timestamp;                   // Время создания
} Commit;

// === Основные функции из ТЗ ===

// Создает пустой репозиторий и возвращает начальный (пустой) коммит
Commit* init_repo(void);

// Принимает текущий коммит, путь к файлу и новое содержимое.
// Возвращает новый коммит, в котором файл добавлен/обновлен.
Commit* add_file(Commit *old_commit, const char *path, const char *content);

// Принимает текущий коммит и путь к файлу.
// Возвращает новый коммит, в котором файл удалён.
Commit* remove_file(Commit *old_commit, const char *path);

// Принимает текущее состояние и сообщение коммита.
// Создаёт финальный коммит с вычисленным хешем.
Commit* commit(Commit *state, const char *message);

// Принимает коммит и путь к файлу.
// Возвращает содержимое файла или NULL.
char* get_file_content(Commit *commit, const char *path, size_t *out_len);

// Принимает коммит и путь.
// Возвращает true/false — есть ли файл в этой версии.
bool get_file_exists(Commit *commit, const char *path);

// Выводит информацию об одном коммите
void print_commit(Commit *commit);

// Принимает любой коммит и выводит всю цепочку истории
void print_history(Commit *commit);

// Принимает коммит и выводит список всех файлов
void print_files(Commit *commit);

// === Вспомогательные функции ===

// Создать коммит из дерева (низкоуровневая функция)
Commit* commit_create(TreeNode *tree, const char *message, const char *parent_hash);

// Загрузить коммит по хешу
Commit* commit_load(const char *hash);

// Сохранить коммит на диск
MinigitStatus commit_save(Commit *commit);

// Освободить память
void commit_free(Commit *commit);

// Получить хеш текущего HEAD
char* get_head_commit(void);

// Обновить HEAD
MinigitStatus update_head(const char *commit_hash);

#endif