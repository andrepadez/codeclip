#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int process_file(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag != FTW_F) return 0;
    if (should_skip(path)) return 0;

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
