#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>  
#include <dirent.h>
#include "helpers.h"
#include "config_manager.h"
#include "clipboard.h"
#include "bfs_traversal.h"

extern char target_dir[MAX_PATH];
extern char project_root[MAX_PATH];
extern char project_name[128];
extern int has_git;
extern FILE *outfile;

int init_config(struct Config *cfg) {
    if (load_config(cfg) < 0) {
        fprintf(stderr, "Error: failed to load config.\n");
        return -1;
    }
    mkdir(cfg->output_dir, 0755);
    return 0;
}

int should_skip(const char *path) {
    return strstr(path, "/.git") ||
           strstr(path, "/node_modules") ||
           strstr(path, "/components/ui");
}

const char *lang_for(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "";

    if (!strcmp(dot, ".c")) return "c";
    else if (!strcmp(dot, ".h")) return "c";
    else if (!strcmp(dot, ".cpp")) return "cpp";
    else if (!strcmp(dot, ".js")) return "javascript";
    else if (!strcmp(dot, ".jsx")) return "javascript";
    else if (!strcmp(dot, ".ts")) return "ts";
    else if (!strcmp(dot, ".tsx")) return "ts";
    else if (!strcmp(dot, ".py")) return "python";
    else if (!strcmp(dot, ".sh")) return "bash";
    else if (!strcmp(dot, ".html")) return "html";
    else if (!strcmp(dot, ".css")) return "css";
    else if (!strcmp(dot, ".json")) return "json";
    else if (!strcmp(dot, ".md")) return "markdown";
    else return "";
}

int find_git_root(const char *start_path, char *git_root, size_t size, char *project_name, size_t name_size) {
    char path[MAX_PATH];
    realpath(start_path, path);

    while (1) {
        char test[MAX_PATH];
        int n = snprintf(test, sizeof(test), "%s/.git", path);
        if (n < 0 || n >= (int)sizeof(test)) {
            // path too long, skip this iteration
            continue;
        }

        struct stat st;
        if (stat(test, &st) == 0 && S_ISDIR(st.st_mode)) {
            strncpy(git_root, path, size - 1);
            git_root[size - 1] = '\0';

            // extract last folder name (project name)
            const char *last_slash = strrchr(path, '/');
            if (last_slash)
                strncpy(project_name, last_slash + 1, name_size - 1);
            else
                strncpy(project_name, path, name_size - 1);
            project_name[name_size - 1] = '\0';

            return 1; // found
        }

        // Stop if we reached HOME or /
        const char *home = getenv("HOME");
        if (strcmp(path, "/") == 0 || (home && strcmp(path, home) == 0))
            break;

        // Move one directory up
        char *slash = strrchr(path, '/');
        if (!slash) break;
        if (slash == path) *(slash + 1) = '\0';
        else *slash = '\0';
    }

    git_root[0] = '\0';
    project_name[0] = '\0';
    return 0;
}

// === Helper: clipboard post-process ===
void maybe_copy_to_clipboard(const char *outfile_path, struct Config *cfg, int clipboard_mode) {
    if (!cfg->clipboard_tool[0]) return;

    if (clipboard_mode) {
        // Old behavior — copy full file contents
        if (copy_to_clipboard(outfile_path, cfg->clipboard_tool) == 0)
            printf("[codeclip] Copied file contents using '%s'\n", cfg->clipboard_tool);
        else
            fprintf(stderr, "[codeclip] Warning: failed to copy contents.\n");
    } else {
        // New default — copy just the file path
        char cmd[MAX_PATH + 64];
        snprintf(cmd, sizeof(cmd), "echo '%s' | %s", outfile_path, cfg->clipboard_tool);
        int ret = system(cmd);
        if (ret == 0)
            printf("[codeclip] Copied file path to clipboard: %s\n", outfile_path);
        else
            fprintf(stderr, "[codeclip] Warning: failed to copy file path.\n");
    }
}

// === Helper: detect project info from target path ===
void detect_project(const char *target) {
    has_git = find_git_root(
        target,
        project_root, sizeof(project_root),
        project_name, sizeof(project_name)
    );

    if (has_git) {
        printf("[codeclip] Detected project root: %s\n", project_root);
        printf("[codeclip] Project name: %s\n", project_name);
    } else {
        const char *last_slash = strrchr(target, '/');
        if (last_slash)
            strncpy(project_name, last_slash + 1, sizeof(project_name) - 1);
        else
            strncpy(project_name, target, sizeof(project_name) - 1);
        project_name[sizeof(project_name) - 1] = '\0';
    }
}

// === Helper: build timestamped output filename ===
void build_output_path(char *outfile_path, size_t size,
                              const char *base_dir, const char *target) {
    outfile_path[0] = '\0';
    strncpy(outfile_path, base_dir, size - 1);
    outfile_path[size - 1] = '\0';

    size_t len = strlen(outfile_path);
    if (len + 1 < size)
        strncat(outfile_path, "/", size - len - 1);

    // --- compute relative or safe path ---
    const char *rel = target;
    if (has_git && strstr(target, project_root))
        rel = target + strlen(project_root);
    if (rel[0] == '/')
        rel++;

    char safe_rel[MAX_PATH];
    snprintf(safe_rel, sizeof(safe_rel), "%s", rel);
    for (char *p = safe_rel; *p; p++)
        if (*p == '/')
            *p = '_';

    // --- timestamp ---
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);

    len = strlen(outfile_path);
    snprintf(outfile_path + len, size - len, "%.31s_%.255s.md", timestamp, safe_rel);
}

// === Helper: write single file ===
int write_single_file(const char *path, struct Config *cfg, int clipboard_mode) {
    char outfile_path[MAX_PATH];
    build_output_path(outfile_path, sizeof(outfile_path), cfg->output_dir, path);

    outfile = fopen(outfile_path, "w");
    if (!outfile) {
        perror("fopen output");
        return -1;
    }

    fprintf(outfile, "# File: %s\n\n", path);

    struct stat st;
    if (stat(path, &st) != 0) {
        perror("stat");
        fclose(outfile);
        return -1;
    }

    struct FTW dummy = { .base = 0, .level = 0 };
    process_file(path, &st, FTW_F, &dummy);

    fclose(outfile);
    printf("[codeclip] Wrote single file to %s\n", outfile_path);

    maybe_copy_to_clipboard(outfile_path, cfg, clipboard_mode);
    return 0;
}

// === Helper: write directory ===
int write_directory(const char *path, struct Config *cfg, int clipboard_mode) {
    char outfile_path[MAX_PATH];
    build_output_path(outfile_path, sizeof(outfile_path), cfg->output_dir, path);

    outfile = fopen(outfile_path, "w");
    if (!outfile) {
        perror("fopen output");
        return -1;
    }

    fprintf(outfile, "# Project: %s\n\n", project_name);

    if (bfs_traverse(path, process_file) != 0) {
        fprintf(stderr, "Error during file traversal.\n");
        fclose(outfile);
        return -1;
    }

    fclose(outfile);
    printf("[codeclip] Output written to %s\n", outfile_path);

    maybe_copy_to_clipboard(outfile_path, cfg, clipboard_mode);
    return 0;
}

int clear_clips(const struct Config *cfg, int force) {
    const char *dir = cfg->output_dir;
    DIR *d = opendir(dir);
    if (!d) {
        perror("opendir");
        return -1;
    }

    // Confirm if not forced
    if (!force) {
        printf("[codeclip] This will delete all files in: %s\n", dir);
        printf("Proceed? (y/N): ");
        char response[8];
        if (!fgets(response, sizeof(response), stdin) ||
            (response[0] != 'y' && response[0] != 'Y')) {
            printf("[codeclip] Cancelled.\n");
            closedir(d);
            return 0;
        }
    }

    struct dirent *entry;
    int count = 0;
    char path[MAX_PATH];

    while ((entry = readdir(d))) {
        if (entry->d_name[0] == '.') continue;  // skip hidden
        snprintf(path, sizeof(path), "%.2047s/%.255s", dir, entry->d_name);
        if (unlink(path) == 0)
            count++;
    }

    closedir(d);
    printf("[codeclip] Deleted %d file(s) from %s\n", count, dir);
    return 0;
}

void print_version(void) {
    printf("codeclip %s\n", CODECLIP_VERSION);
}

void print_help(void) {
    printf("codeclip %s\n", CODECLIP_VERSION);
    printf("Usage:\n");
    printf("  codeclip <path> [options]\n");
    printf("  codeclip <subcommand> [options]\n\n");
    printf("Options:\n");
    printf("  -c, --clipboard    Copy full markdown content instead of file path\n");
    printf("  -h, --help         Show this help message\n");
    printf("  -v, --version      Show version information\n\n");
    printf("Subcommands:\n");
    printf("  clear              Delete all generated markdown files\n");
    printf("  version            Show version information\n");
    printf("  help               Show this help message\n\n");
    printf("Examples:\n");
    printf("  codeclip src/                    # Dump a project directory\n");
    printf("  codeclip -c src/main.c           # Dump a single file, copy full markdown\n");
    printf("  codeclip clear                   # Clear saved clips\n");
    printf("  codeclip version                 # Show version\n\n");
}
