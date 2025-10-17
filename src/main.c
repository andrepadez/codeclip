#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>
#include "helpers.h"
#include "config_manager.h"
#include "bfs_traversal.h"
#include "clipboard.h"

char target_dir[MAX_PATH];
char project_root[MAX_PATH];
char project_name[128];
int has_git = 0;
FILE *outfile = NULL;

// === MAIN ===
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }

    struct Config cfg;
    if (init_config(&cfg) != 0)
        return 1;

    if (!realpath(argv[1], target_dir)) {
        perror("realpath");
        return 1;
    }

    struct stat st;
    if (stat(target_dir, &st) != 0) {
        perror("stat");
        return 1;
    }

    detect_project(target_dir);

    if (S_ISREG(st.st_mode))
        return write_single_file(target_dir, &cfg);
    else if (S_ISDIR(st.st_mode))
        return write_directory(target_dir, &cfg);
    else {
        fprintf(stderr, "Error: '%s' is not a valid file or directory\n", target_dir);
        return 1;
    }
}
