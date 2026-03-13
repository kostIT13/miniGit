#include <stdio.h>
#include <string.h>
#include "repo.h"
#include "minigit.h"

void print_usage(const char *prog_name) {
    printf("Usage: %s <command>\n", prog_name);
    printf("Commands:\n");
    printf("  init    Initialize a new repository\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    MinigitStatus status = MINIGIT_OK;

    if (strcmp(argv[1], "init") == 0) {
        status = init_repo();
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

    return (status == MINIGIT_OK) ? 0 : 1;
}