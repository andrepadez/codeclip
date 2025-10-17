#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ftw.h>
#include <sys/stat.h>
#include "helpers.h"

extern FILE *outfile;
extern char target_dir[MAX_PATH];
extern char project_root[MAX_PATH];
extern char project_name[128];
extern int has_git;

// Skip certain directories and files (defined in helpers.c)
int should_skip(const char *path);

// --- Detect binary or non-text file ---
static int is_binary_file(const char *path) {
    // --- Fast extension check ---
    static const char *binary_exts[] = {
        // archives & compressed
        ".zip", ".tar", ".gz", ".bz2", ".xz", ".7z", ".rar",
        // images
        ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tiff", ".ico", ".webp", ".svgz",
        // audio / video
        ".mp3", ".wav", ".flac", ".ogg", ".mp4", ".mkv", ".avi", ".mov", ".webm",
        // fonts
        ".ttf", ".otf", ".woff", ".woff2",
        // executables / libs / bytecode
        ".exe", ".dll", ".so", ".o", ".a", ".class", ".jar", ".wasm",
        // databases / binary data
        ".db", ".sqlite", ".bin", ".dat", ".pak", ".cache", ".npy", ".npz",
        // others
        ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
        NULL
    };

    const char *dot = strrchr(path, '.');
    if (dot) {
        for (int i = 0; binary_exts[i]; i++) {
            if (strcasecmp(dot, binary_exts[i]) == 0)
                return 1; // match: definitely binary
        }
    }

    // --- Heuristic content-based check ---
    FILE *f = fopen(path, "rb");
    if (!f) return 1; // treat unreadable as binary

    unsigned char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    if (n == 0) return 0; // empty file = fine

    int nonprint = 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = buf[i];
        if (c == 0) return 1; // null byte = binary
        if (!isprint(c) && !isspace(c)) {
            nonprint++;
            if (nonprint > 8)
                return 1;
        }
    }

    return 0; // looks like text
}

int process_file(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag != FTW_F) return 0;
    if (should_skip(path)) return 0;
    if (is_binary_file(path)) return 0;

    const char *base = has_git ? project_root : target_dir;
    const char *rel_path = path + strlen(base);
    if (*rel_path == '/') rel_path++;  // skip leading slash

    // --- Markdown section header ---
    fprintf(outfile, "--- %s/%s ---\n", project_name, rel_path);

    // --- Guess language for Markdown code fences ---
    const char *dot = strrchr(path, '.');
    const char *lang = "";
    if (dot) {
        if (!strcmp(dot, ".c") || !strcmp(dot, ".h")) lang = "c";
        else if (!strcmp(dot, ".cpp")) lang = "cpp";
        else if (!strcmp(dot, ".js") || !strcmp(dot, ".jsx")) lang = "javascript";
        else if (!strcmp(dot, ".ts") || !strcmp(dot, ".tsx")) lang = "ts";
        else if (!strcmp(dot, ".py")) lang = "python";
        else if (!strcmp(dot, ".sh")) lang = "bash";
        else if (!strcmp(dot, ".html")) lang = "html";
        else if (!strcmp(dot, ".css")) lang = "css";
        else if (!strcmp(dot, ".json")) lang = "json";
        else if (!strcmp(dot, ".md")) lang = "markdown";
    }

    fprintf(outfile, "```%s\n", lang);

    // --- Dump file contents ---
    FILE *f = fopen(path, "r");
    if (f) {
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
            fwrite(buf, 1, n, outfile);
        fclose(f);
    }

    fprintf(outfile, "\n```\n\n");
    return 0;
}
