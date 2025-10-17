#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <ftw.h>

#include "helpers.h"
#include "config_manager.h"
#include "clipboard.h"

#define OUTDIR_NAME "codeclips"

char target_dir[MAX_PATH];
FILE *outfile = NULL;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }

    struct Config cfg;
    load_config(&cfg);       // ✅ Ensures ~/.config/codeclip exists
    mkdir(cfg.output_dir, 0755);

    realpath(argv[1], target_dir);
    struct stat st;
    if (stat(target_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a directory\n", target_dir);
        return 1;
    }

    // Create output file with timestamp
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", tm);

    char outfile_path[MAX_PATH];
    if (snprintf(outfile_path, sizeof(outfile_path),
                 "%s/%s_codeclip.md", cfg.output_dir, timestamp) >= (int)sizeof(outfile_path)) {
        fprintf(stderr, "Error: output path too long.\n");
        return 1;
    }

    outfile = fopen(outfile_path, "w");
    if (!outfile) {
        perror("fopen");
        return 1;
    }

    nftw(target_dir, process_file, 32, FTW_PHYS);
    fclose(outfile);

    // ✅ Unified clipboard logic
    copy_to_clipboard(outfile_path, cfg.clipboard_tool);

    printf("Code from '%s' saved to:\n  %s\n", target_dir, outfile_path);
    return 0;
}
