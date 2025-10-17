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
        fprintf(stderr, "Usage: %s <path|clear|help|version> [options]\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -c, --clipboard   Copy full markdown content instead of file path\n");
        return 1;
    }

    // --- Handle help flags early ---
    if (strcmp(argv[1], "help") == 0 ||
        strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    // --- Detect clipboard mode flag (-c / --clipboard) ---
    int clipboard_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--clipboard") == 0)
            clipboard_mode = 1;
    }

    // --- Handle version flags early ---
    if (strcmp(argv[1], "version") == 0 ||
        strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0) {
        print_version();
        return 0;
    }

    struct Config cfg;
    if (init_config(&cfg) != 0)
        return 1;

    // --- Handle subcommands ---
    if (strcmp(argv[1], "clear") == 0) {
        int force = (argc > 2 && strcmp(argv[2], "--force") == 0);
        clear_clips(&cfg, force);
        return 0;
    }

    // --- Find first non-flag argument as target path ---
    const char *target_arg = NULL;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {  // first argument that isn't a flag
            target_arg = argv[i];
            break;
        }
    }

    if (!target_arg) {
        fprintf(stderr, "Error: no path provided.\n");
        return 1;
    }

    // --- Resolve target path ---
    if (!realpath(target_arg, target_dir)) {
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
        return write_single_file(target_dir, &cfg, clipboard_mode);
    else if (S_ISDIR(st.st_mode))
        return write_directory(target_dir, &cfg, clipboard_mode);
    else {
        fprintf(stderr, "Error: '%s' is not a valid file or directory\n", target_dir);
        return 1;
    }
}

