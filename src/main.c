#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <ftw.h>

#include "helpers.h"
#include "config_manager.h"

#define OUTDIR_NAME "codeclips"

char target_dir[MAX_PATH];
FILE *outfile = NULL;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }

    struct Config cfg;
    load_config(&cfg);                     // ✅ Create & read ~/.config/codeclip

    // Make sure output directory exists
    mkdir(cfg.output_dir, 0755);

    // Resolve the input directory
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
    if (snprintf(outfile_path, sizeof(outfile_path), "%s/%s_codeclip.md",
                 cfg.output_dir, timestamp) >= (int)sizeof(outfile_path)) {
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

    // Copy to clipboard
    if (cfg.clipboard_tool[0]) {
        char cmd[MAX_PATH * 2];
        snprintf(cmd, sizeof(cmd), "%s < '%s' 2>/dev/null", cfg.clipboard_tool, outfile_path);
        system(cmd);
        printf("📋 Code copied using custom tool: %s\n", cfg.clipboard_tool);
    } else {
        const char *tools[] = { "xclip -selection clipboard", "wl-copy", "pbcopy" };
        for (int i = 0; i < 3; i++) {
            char cmd[MAX_PATH * 2];
            snprintf(cmd, sizeof(cmd), "%s < '%s' 2>/dev/null", tools[i], outfile_path);
            if (system(cmd) == 0) {
                printf("📋 Code copied using %s\n", tools[i]);
                break;
            }
        }
    }

    printf("Code from '%s' saved to:\n  %s\n", target_dir, outfile_path);
    return 0;
}
