#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>   // for basename()
#include <time.h>     // ✅ for time(), localtime(), strftime()
#include "helpers.h"
#include "config_manager.h"
#include "bfs_traversal.h"
#include "clipboard.h"

char target_dir[MAX_PATH];
char project_root[MAX_PATH];
char project_name[128];
int has_git = 0;
FILE *outfile = NULL;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }

    struct Config cfg;
    if (load_config(&cfg) < 0) {
        fprintf(stderr, "Error: failed to load config.\n");
        return 1;
    }
    mkdir(cfg.output_dir, 0755);

    if (!realpath(argv[1], target_dir)) {
        perror("realpath");
        return 1;
    }

    struct stat st;
    if (stat(target_dir, &st) != 0) {
        perror("stat");
        return 1;
    }

    // --- Detect Git root (if applicable) ---
    has_git = find_git_root(
        target_dir,
        project_root, sizeof(project_root),
        project_name, sizeof(project_name)
    );

    if (has_git) {
        printf("[codeclip] Detected project root: %s\n", project_root);
        printf("[codeclip] Project name: %s\n", project_name);
    } else {
        const char *last_slash = strrchr(target_dir, '/');
        if (last_slash)
            strncpy(project_name, last_slash + 1, sizeof(project_name) - 1);
        else
            strncpy(project_name, target_dir, sizeof(project_name) - 1);
        project_name[sizeof(project_name) - 1] = '\0';
    }

    char outfile_path[MAX_PATH];
    outfile_path[0] = '\0';
    strncpy(outfile_path, cfg.output_dir, sizeof(outfile_path) - 1);
    outfile_path[sizeof(outfile_path) - 1] = '\0';

    size_t len = strlen(outfile_path);
    if (len + 1 < sizeof(outfile_path))
        strncat(outfile_path, "/", sizeof(outfile_path) - len - 1);

    // --- Handle single-file mode ---
    if (S_ISREG(st.st_mode)) {
        // --- Compute relative path from project root (if available) ---
        const char *rel = target_dir;
        if (has_git && strstr(target_dir, project_root))
            rel = target_dir + strlen(project_root);
        if (rel[0] == '/') rel++; // skip leading slash

        // --- Replace '/' with '_' to make it safe ---
        char safe_rel[MAX_PATH];
        snprintf(safe_rel, sizeof(safe_rel), "%s", rel);
        for (char *p = safe_rel; *p; p++)
            if (*p == '/')
                *p = '_';

        // --- Add timestamp prefix ---
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);

        // --- Build final output filename ---
        len = strlen(outfile_path);
        snprintf(outfile_path + len, sizeof(outfile_path) - len,
         "%.31s_%.255s.md", timestamp, safe_rel);

        outfile = fopen(outfile_path, "w");
        if (!outfile) {
            perror("fopen output");
            return 1;
        }

        fprintf(outfile, "# File: %s\n\n", target_dir);

        struct FTW dummy = { .base = 0, .level = 0 };
        process_file(target_dir, &st, FTW_F, &dummy);

        fclose(outfile);
        printf("[codeclip] Wrote single file to %s\n", outfile_path);
    }
    
    else if (S_ISDIR(st.st_mode)) {
        // --- Directory traversal mode ---
        const char *rel = target_dir;
        if (has_git && strstr(target_dir, project_root))
            rel = target_dir + strlen(project_root);

        char safe_rel[MAX_PATH];
        snprintf(safe_rel, sizeof(safe_rel), "%s", rel[0] == '/' ? rel + 1 : rel);
        for (char *p = safe_rel; *p; p++)
            if (*p == '/')
                *p = '_';

        len = strlen(outfile_path);
        snprintf(outfile_path + len, sizeof(outfile_path) - len,
                 "%.127s_%.255s.md", project_name, safe_rel);

        outfile = fopen(outfile_path, "w");
        if (!outfile) {
            perror("fopen output");
            return 1;
        }

        fprintf(outfile, "# Project: %s\n\n", project_name);

        if (bfs_traverse(target_dir, process_file) != 0) {
            fprintf(stderr, "Error during file traversal.\n");
            fclose(outfile);
            return 1;
        }

        fclose(outfile);
        printf("[codeclip] Output written to %s\n", outfile_path);
    } else {
        fprintf(stderr, "Error: '%s' is not a regular file or directory\n", target_dir);
        return 1;
    }

    // --- Copy to clipboard if configured ---
    if (cfg.clipboard_tool[0]) {
        if (copy_to_clipboard(outfile_path, cfg.clipboard_tool) == 0) {
            printf("[codeclip] Copied to clipboard using '%s'\n", cfg.clipboard_tool);
        } else {
            fprintf(stderr, "[codeclip] Warning: failed to copy to clipboard.\n");
        }
    }

    return 0;
}
