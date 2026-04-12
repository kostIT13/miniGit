#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repo.h"
#include "minigit.h"
#include "blob.h"
#include "tree.h"
#include "commit.h"

void print_usage(const char *prog_name) {
    printf("Usage: %s <command> [arguments]\n", prog_name);
    printf("\nCommands:\n");
    printf("  init_repo                          Initialize a new repository\n");
    printf("  add_file <path> <content>          Add/update file in current commit\n");
    printf("  remove_file <path>                 Remove file from current commit\n");
    printf("  commit <message>                   Create final commit with message\n");
    printf("  get_file_content <path>            Show file content from HEAD\n");
    printf("  get_file_exists <path>             Check if file exists in HEAD\n");
    printf("  print_commit [hash]                Show commit info (HEAD if no hash)\n");
    printf("  print_history                      Show commit history\n");
    printf("  print_files                        List all files in current commit\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    int status = 0;

    if (strcmp(argv[1], "init_repo") == 0) {
        Commit *repo_commit = init_repo();
        if (repo_commit) {
            printf("Repository initialized. HEAD: %s\n", repo_commit->hash);
            commit_free(repo_commit);
        } else {
            fprintf(stderr, "Failed to initialize repository.\n");
            status = 1;
        }
    }

    else if (strcmp(argv[1], "add_file") == 0) {
        if (argc < 4) {
            printf("Usage: %s add_file <filepath> <content>\n", argv[0]);
            return 1;
        }

        const char *filepath = argv[2];
        const char *content = argv[3];

        Commit *current = init_repo();
        if (!current) {
            printf("Error: No repository found. Run 'init_repo' first.\n");
            return 1;
        }

        Commit *new_commit = add_file(current, filepath, content);

        if (new_commit) {
            printf("[OK] File added. New commit: %s\n", new_commit->hash);
            update_head(new_commit->hash);
            commit_free(current);
            commit_free(new_commit);
        } else {
            printf("[FAIL] Failed to add file\n");
            status = 1;
        }
    }

    else if (strcmp(argv[1], "remove_file") == 0) {
        if (argc < 3) {
            printf("Usage: %s remove_file <filepath>\n", argv[0]);
            return 1;
        }

        const char *filepath = argv[2];

        Commit *current = init_repo();
        if (!current) {
            printf("Error: No repository found.\n");
            return 1;
        }

        Commit *new_commit = remove_file(current, filepath);

        if (new_commit) {
            if (new_commit == current) {
                printf("[INFO] File did not exist\n");
            } else {
                printf("[OK] File removed. New commit: %s\n", new_commit->hash);
                update_head(new_commit->hash);
                commit_free(new_commit);
            }
            commit_free(current);
        } else {
            printf("[FAIL] Failed to remove file\n");
            status = 1;
        }
    }

    else if (strcmp(argv[1], "commit") == 0) {
        if (argc < 3) {
            printf("Usage: %s commit <message>\n", argv[0]);
            return 1;
        }

        const char *message = argv[2];

        Commit *current = init_repo();
        if (!current) {
            printf("Error: No repository found.\n");
            return 1;
        }

        Commit *new_commit = commit(current, message);

        if (new_commit) {
            printf("[OK] Commit created: %s\n", new_commit->hash);
            commit_free(current);
            commit_free(new_commit);
        } else {
            printf("[FAIL] Failed to create commit\n");
            status = 1;
        }
    }

    else if (strcmp(argv[1], "get_file_content") == 0) {
        if (argc < 3) {
            printf("Usage: %s get_file_content <filepath>\n", argv[0]);
            return 1;
        }

        const char *filepath = argv[2];

        char *head_hash = get_head_commit();
        if (!head_hash) {
            printf("Error: No commits yet.\n");
            return 1;
        }

        Commit *commit = commit_load(head_hash);
        free(head_hash);

        if (!commit) {
            printf("Error: Failed to load commit.\n");
            return 1;
        }

        size_t len = 0;
        char *content = get_file_content(commit, filepath, &len);

        if (content) {
            printf("%s", content);
            free(content);
        } else {
            printf("Error: File '%s' not found in current commit.\n", filepath);
            status = 1;
        }

        commit_free(commit);
    }

    else if (strcmp(argv[1], "get_file_exists") == 0) {
        if (argc < 3) {
            printf("Usage: %s get_file_exists <filepath>\n", argv[0]);
            return 1;
        }

        const char *filepath = argv[2];

        char *head_hash = get_head_commit();
        if (!head_hash) {
            printf("false\n");
            return 1;
        }

        Commit *commit = commit_load(head_hash);
        free(head_hash);

        if (!commit) {
            printf("false\n");
            return 1;
        }

        bool exists = get_file_exists(commit, filepath);
        printf("%s\n", exists ? "true" : "false");

        commit_free(commit);
        status = exists ? 0 : 1;
    }

    else if (strcmp(argv[1], "print_commit") == 0) {
        char *commit_hash = NULL;

        if (argc >= 3) {
            commit_hash = strdup(argv[2]);
        } else {
            commit_hash = get_head_commit();
        }

        if (!commit_hash) {
            printf("Error: No commits yet.\n");
            return 1;
        }

        Commit *commit = commit_load(commit_hash);
        free(commit_hash);

        if (!commit) {
            printf("Error: Failed to load commit.\n");
            return 1;
        }

        print_commit(commit);
        commit_free(commit);
    }

    else if (strcmp(argv[1], "print_history") == 0) {
        char *head_hash = get_head_commit();

        if (!head_hash) {
            printf("No commits yet.\n");
            return 1;
        }

        Commit *commit = commit_load(head_hash);
        free(head_hash);

        if (!commit) {
            printf("Error: Failed to load head commit.\n");
            return 1;
        }

        printf("=== Commit History ===\n\n");
        print_history(commit);
    }

    else if (strcmp(argv[1], "print_files") == 0) {
        char *commit_hash = get_head_commit();

        if (!commit_hash) {
            printf("No commits yet.\n");
            return 1;
        }

        Commit *commit = commit_load(commit_hash);
        free(commit_hash);

        if (!commit) {
            printf("Error: Failed to load commit.\n");
            return 1;
        }

        print_files(commit);
        commit_free(commit);
    }

    else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

    return status == 0 ? 0 : 1;
}