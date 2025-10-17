#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include "helpers.h"

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
