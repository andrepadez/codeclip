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
#include "bfs_traversal.h"

#define OUTDIR_NAME "codeclips"

char project_root[MAX_PATH];
char project_name[128];
int has_git = 0;

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

    // Resolve full target path
    realpath(argv[1], target_dir);
    struct stat st;
    if (stat(target_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a directory\n", target_dir);
        return 1;
    }

    // Detect .git project root and name
    has_git = find_git_root(
        target_dir,
        project_root, sizeof(project_root),
        project_name, sizeof(project_name)
    );

    if (has_git) {
        printf("[codeclip] Detected project root: %s\n", project_root);
        printf("[codeclip] Project name: %s\n", project_name);
    } else {
        printf("[codeclip] No .git found — using given directory as base.\n");
        const char *slash = strrchr(target_dir, '/');
        if (slash)
            strncpy(project_name, slash + 1, sizeof(project_name) - 1);
        else
            strncpy(project_name, target_dir, sizeof(project_name) - 1);
        project_name[sizeof(project_name) - 1] = '\0';
    }

    // Prepare timestamped output file
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

    // Walk and process files
    bfs_traverse(target_dir, process_file);
    fclose(outfile);

    // ✅ Unified clipboard logic
    copy_to_clipboard(outfile_path, cfg.clipboard_tool);

    printf("Code from project '%s' saved to:\n  %s\n",
           project_name, outfile_path);

    return 0;
}
