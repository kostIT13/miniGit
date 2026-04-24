#include "../include/commit.h"
#include "../include/tree.h"
#include "../include/repo.h"
#include "../include/minigit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
    #define CLEANUP_CMD "timeout /T 1 /NOBREAK >nul 2>&1 & rmdir /s /q .minigit 2>nul & mkdir .minigit 2>nul & rmdir /q .minigit 2>nul"
#else
    #define CLEANUP_CMD "rm -rf .minigit"
#endif


#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("[RUN] %-40s ... ", #name); \
    test_##name(); \
    printf("PASSED\n"); \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("\n  FAILED: %s\n  at %s:%d\n", msg, __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != NULL, "pointer is NULL")
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0, "string mismatch")


TEST(basic_init) {
    system(CLEANUP_CMD);
    
    Commit *head = init_repo();
    ASSERT_NOT_NULL(head);
    ASSERT_NOT_NULL(head->tree);
    ASSERT(head->tree->children_count == 0, "tree should be empty");
    ASSERT(strlen(head->hash) == HASH_HEX_LEN, "hash length mismatch");
    
    commit_free(head);
    system(CLEANUP_CMD);
}

TEST(add_and_read_single_file) {
    system(CLEANUP_CMD);
    
    Commit *head = init_repo();
    Commit *staging = add_file(head, "hello.txt", "Hello MiniGit!\n");
    ASSERT_NOT_NULL(staging);
    
    // Читаем обратно
    size_t len = 0;
    char *content = get_file_content(staging, "hello.txt", &len);
    ASSERT_NOT_NULL(content);
    ASSERT_STR_EQ(content, "Hello MiniGit!\n");
    free(content);
    
    // Проверяем exists
    ASSERT(get_file_exists(staging, "hello.txt") == true, "file should exist");
    ASSERT(get_file_exists(staging, "missing.txt") == false, "missing file check");
    
    commit_free(staging);
    commit_free(head);
    system(CLEANUP_CMD);
}

TEST(commit_chain_preserves_parents) {
    system(CLEANUP_CMD);
    
    Commit *c0 = init_repo();
    ASSERT_STR_EQ(c0->parent_hash, "");  // Первый коммит без родителя
    
    Commit *c1 = commit(add_file(c0, "a.txt", "content A"), "First commit");
    ASSERT_NOT_NULL(c1);
    ASSERT_STR_EQ(c1->parent_hash, c0->hash);
    ASSERT(strlen(c1->message) > 0, "message should not be empty");
    
    Commit *c2 = commit(add_file(c1, "b.txt", "content B"), "Second commit");
    ASSERT_NOT_NULL(c2);
    ASSERT_STR_EQ(c2->parent_hash, c1->hash);
    ASSERT(c2 != c1, "commit should create new object");  // Новый объект
    
    // Просто проверяем, что print_history не падает
    print_history(c2);
    
    commit_free(c2);
    commit_free(c1);
    commit_free(c0);
    system(CLEANUP_CMD);
}

TEST(persistence_old_versions_accessible) {
    system(CLEANUP_CMD);
    
    Commit *c0 = init_repo();
    Commit *c1 = commit(add_file(c0, "tmp.txt", "to delete"), "Add tmp");
    
    // Удаляем файл в новой версии
    Commit *c2 = remove_file(c1, "tmp.txt");
    ASSERT_NOT_NULL(c2);
    
    // 🔑 ПЕРСИСТЕНТНОСТЬ: старый коммит НЕ должен меняться
    ASSERT(get_file_exists(c2, "tmp.txt") == false, "file removed in c2");
    ASSERT(get_file_exists(c1, "tmp.txt") == true, "file still in c1");
    
    size_t len = 0;
    char *old_content = get_file_content(c1, "tmp.txt", &len);
    ASSERT_NOT_NULL(old_content);
    ASSERT_STR_EQ(old_content, "to delete");
    free(old_content);
    
    commit_free(c2);
    commit_free(c1);
    commit_free(c0);
    system(CLEANUP_CMD);
}

TEST(persistence_disk_reload) {
    system(CLEANUP_CMD);
    
    // Создаём и коммитим
    Commit *c1 = commit(add_file(init_repo(), "persist.txt", "I survive restart"), "Persistent");
    char original_hash[HASH_HEX_LEN + 1];
    strncpy(original_hash, c1->hash, HASH_HEX_LEN);
    original_hash[HASH_HEX_LEN] = '\0';
    
    // Эмулируем перезапуск: освобождаем и загружаем заново
    commit_free(c1);
    
    Commit *reloaded = init_repo();  // Должен загрузить HEAD с диска
    ASSERT_NOT_NULL(reloaded);
    ASSERT_STR_EQ(reloaded->hash, original_hash);
    
    size_t len = 0;
    char *content = get_file_content(reloaded, "persist.txt", &len);
    ASSERT_NOT_NULL(content);
    ASSERT_STR_EQ(content, "I survive restart");
    free(content);
    
    commit_free(reloaded);
    system(CLEANUP_CMD);
}

TEST(structural_sharing_unchanged_files) {
    system(CLEANUP_CMD);
    
    Commit *c0 = init_repo();
    Commit *c1 = commit(add_file(c0, "shared.txt", "unchanged data"), "v1");
    
    // Обновляем ТОЛЬКО один файл
    Commit *c2 = commit(add_file(c1, "changed.txt", "new version"), "v2");
    
    const char *h1 = tree_get_file_hash(c1->tree, "shared.txt");
    const char *h2 = tree_get_file_hash(c2->tree, "shared.txt");
    
    ASSERT_NOT_NULL(h1);
    ASSERT_NOT_NULL(h2);
    ASSERT_STR_EQ(h1, h2);  // Один и тот же блоб!
    
    // Контент в обеих версиях одинаковый
    size_t len = 0;
    char *data1 = get_file_content(c1, "shared.txt", &len);
    char *data2 = get_file_content(c2, "shared.txt", &len);
    ASSERT_STR_EQ(data1, "unchanged data");
    ASSERT_STR_EQ(data2, "unchanged data");
    free(data1);
    free(data2);
    
    commit_free(c2);
    commit_free(c1);
    commit_free(c0);
    system(CLEANUP_CMD);
}

TEST(remove_nonexistent_file_returns_same) {
    system(CLEANUP_CMD);
    
    Commit *head = init_repo();
    Commit *result = remove_file(head, "does_not_exist.txt");
    
    // Если файла не было, можно вернуть тот же коммит (оптимизация)
    // Проверяем, что хотя бы не упало
    ASSERT_NOT_NULL(result);
    
    commit_free(result);
    if (result != head) commit_free(head);  // Если вернулся новый — освобождаем оба
    system(CLEANUP_CMD);
}

TEST(print_functions_no_crash) {
    system(CLEANUP_CMD);
    
    Commit *c0 = init_repo();
    Commit *c1 = commit(add_file(c0, "test.c", "#include <stdio.h>"), "Add test.c");
    
    // Просто проверяем, что функции не падают
    print_commit(c1);
    printf("\n");
    print_files(c1);
    printf("\n");
    // print_history уже тестировалась выше
    
    commit_free(c1);
    commit_free(c0);
    system(CLEANUP_CMD);
}

int main(int argc, char *argv[]) {
    printf("\n");
    printf("     miniGit Test Suite v1.0          \n");
    
    
    // Можно запускать отдельные тесты по имени
    if (argc > 1) {
        const char *filter = argv[1];
        #define RUN_IF_MATCH(name) if (strcmp(#name, filter) == 0) RUN_TEST(name)
        RUN_IF_MATCH(basic_init);
        RUN_IF_MATCH(add_and_read_single_file);
        RUN_IF_MATCH(commit_chain_preserves_parents);
        RUN_IF_MATCH(persistence_old_versions_accessible);
        RUN_IF_MATCH(persistence_disk_reload);
        RUN_IF_MATCH(structural_sharing_unchanged_files);
        RUN_IF_MATCH(remove_nonexistent_file_returns_same);
        RUN_IF_MATCH(print_functions_no_crash);
        printf("\nWarning: test '%s' not found\n", filter);
        return 1;
    }
    
    // Запуск всех
    RUN_TEST(basic_init);
    RUN_TEST(add_and_read_single_file);
    RUN_TEST(commit_chain_preserves_parents);
    RUN_TEST(persistence_old_versions_accessible);
    RUN_TEST(persistence_disk_reload);
    RUN_TEST(structural_sharing_unchanged_files);
    RUN_TEST(remove_nonexistent_file_returns_same);
    RUN_TEST(print_functions_no_crash);
    
    printf("     ALL TESTS PASSED (%d/8)        \n", 8);
    
    return 0;
}