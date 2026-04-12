#ifndef BLOB_H
#define BLOB_H

#include "minigit.h"

// Сохраняет содержимое в хранилище объектов, возвращает хеш
MinigitStatus blob_save(const void *content, size_t len, char *hash_output);

// Загружает содержимое по хешу
// Возвращает NULL если не найдено
char* blob_load(const char *hash, size_t *out_len);

// Получает путь к файлу объекта по его хешу
// Например: .minigit/objects/ab/cdef...
void get_object_path(const char *hash, char *path_output, size_t path_size);

#endif