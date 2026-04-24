# miniGit 

> Упрощённая персистентная система контроля версий на C, реализующая ключевые принципы Git: неизменяемость версий, структурное разделение, хэширование содержимого и цепочку коммитов.

[![C](https://img.shields.io/badge/language-C-blue.svg)](https://en.cppreference.com/w/c)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Build](https://img.shields.io/badge/build-Make-lightgrey.svg)](Makefile)

---

## Оглавление

- [Что такое miniGit?](-что-такое-minigit)
- [Ключевые концепции](-ключевые-концепции)
- [Структура проекта](-структура-проекта)
- [Сборка и запуск](-сборка-и-запуск)
- [API справочник](-api-справочник)
- [Тестирование](-тестирование)

---

## Что такое miniGit?

**miniGit** — это учебная реализация системы контроля версий, написанная на чистом C.


### Цели проекта

1. **Понимание персистентности** — любая предыдущая версия остаётся неизменной
2. **Структурное разделение** — неизменённые файлы не копируются, а используются по ссылке
3. **Хэширование содержимого** — адресация объектов по хэшу, а не по имени
4. **Цепочка коммитов** — каждый коммит ссылается на родителя, образуя историю

---

##  Ключевые концепции

### Персистентность (Неизменяемость)

```c
Commit *c1 = add_file(head, "file.txt", "v1");
Commit *c2 = add_file(c1, "file.txt", "v2");

// c1 и c2 — разные объекты в памяти
// При изменении c2, c1 остаётся неизменным
assert(strcmp(get_file_content(c1, "file.txt"), "v1") == 0);  // ✓
assert(strcmp(get_file_content(c2, "file.txt"), "v2") == 0);  // ✓
```

### Структурное разделение (Structural Sharing)
```bash
Коммит 1: [fileA@hash1] [fileB@hash2]
                    │
Коммит 2: [fileA@hash1] [fileB@hash3]  ← fileA НЕ копируется!
                    │
Коммит 3: [fileA@hash1] [fileC@hash4]  ← fileA всё ещё общий!
```

* Неизменённые файлы хранятся один раз в пуле блобов
* Разные коммиты ссылаются на одни и те же хэши
* Экономия памяти: при 100 коммитах с 10 файлами, где меняется 1 файл — хранится ~110 блобов, а не 1000

### Хэширование содержимого
```bash
// Хэш вычисляется от содержимого, а не от имени файла
char hash[HASH_HEX_LEN + 1];
sha1_hash("Hello World", 11, hash);  // → "a1b2c3d4..."

// Одинаковое содержимое = одинаковый хэш = один объект
blob_save("test", 4, hash1);  // hash1 = "9845fde1"
blob_save("test", 4, hash2);  // hash2 = "9845fde1" (тот же!)
```

### Цепочка коммитов

```bash
commit abc123... (HEAD -> master)
Author: miniGit
Date:   2024-04-24

    Add feature X

commit def456...
Author: miniGit  
Date:   2024-04-23

    Initial commit
```

## Структура проекта

```bash
miniGit/
├── include/                    # Публичные заголовки
│   ├── minigit.h              # Общие константы и типы
│   ├── commit.h               # API коммитов
│   ├── tree.h                 # API дерева файлов
│   ├── blob.h                 # API хранилища содержимого
│   ├── hash.h                 # API хэширования
│   └── repo.h                 # API репозитория
│
├── src/                       # Исходный код
│   ├── main.c                 # CLI интерфейс
│   ├── commit.c               # Реализация коммитов
│   ├── tree.c                 # Реализация дерева (structural sharing)
│   ├── blob.c                 # Хранение блобов с дедупликацией
│   ├── hash.c                 # Функции хэширования
│   └── repo.c                 # Инициализация и работа с .minigit/
│
├── tests/                     # Тесты
│   └── test_minigit.c         # Полный набор модульных тестов
│
├── obj/                       # Объектные файлы (генерируется)
├── .gitignore                 # Исключения для настоящего Git
├── Makefile                   # Сборка через make
└── README.md                  # Этот файл
```

## Сборка и запуск

### Требования

* Компилятор C99+: gcc, clang, MSVC
* Утилита make (или ручная компиляция)
* Windows: MinGW или Visual Studio с поддержкой C99

### Сборка
```bash
# Linux/macOS
make clean
make

# Windows (MinGW)
mingw32-make clean
mingw32-make

# Windows (MSVC)
nmake -f Makefile.msvc
```

### Базовое использование (через CLI)
```bash
# Инициализация репозитория
./minigit.exe init_repo

# Добавление файла
./minigit.exe add_file readme.txt "Hello MiniGit!"

# Фиксация коммита
./minigit.exe commit "Initial commit"

# Просмотр истории
./minigit.exe print_history

# Чтение файла из текущей версии
./minigit.exe get_file_content readme.txt
```

# API Справочник miniGit

## Основные функции (из ТЗ)

| Функция | Описание | Параметры | Возвращает |
|---------|----------|-----------|------------|
| `Commit* init_repo(void)` | Создаёт пустой репозиторий и возвращает начальный коммит | — | `Commit*` — начальный коммит или `NULL` |
| `Commit* add_file(Commit *old_commit, const char *path, const char *content)` | Добавляет/обновляет файл в новой версии | `old_commit` — текущий коммит<br>`path` — путь к файлу<br>`content` — содержимое | `Commit*` — новый коммит (старый неизменен) |
| `Commit* remove_file(Commit *old_commit, const char *path)` | Удаляет файл из новой версии | `old_commit` — текущий коммит<br>`path` — путь к файлу | `Commit*` — новый коммит или тот же, если файла не было |
| `Commit* commit(Commit *state, const char *message)` | Фиксирует состояние с сообщением | `state` — промежуточное состояние<br>`message` — сообщение коммита | `Commit*` — финальный коммит с хэшем |
| `char* get_file_content(Commit *commit, const char *path, size_t *out_len)` | Получает содержимое файла из версии | `commit` — коммит<br>`path` — путь к файлу<br>`out_len` — длина контента (опционально) | `char*` — строка с контентом или `NULL` |
| `bool get_file_exists(Commit *commit, const char *path)` | Проверяет наличие файла в версии | `commit` — коммит<br>`path` — путь к файлу | `bool` — `true`/`false` |
| `void print_commit(Commit *commit)` | Выводит информацию об одном коммите | `commit` — коммит | — |
| `void print_history(Commit *commit)` | Выводит цепочку коммитов назад до начального | `commit` — любой коммит | — |
| `void print_files(Commit *commit)` | Выводит список всех файлов в версии | `commit` — коммит | — |



## Тестирование

### Запуск всех тестов
```bash
# Сборка и запуск
make test

# Очистка тестовых артефактов
make test-clean
```

### Запуск отдельного теста
```bash
# Сборка тестового бинарника
make test_minigit.exe

# Запуск конкретного теста (по имени функции)
./test_minigit.exe structural_sharing_unchanged_files
```


