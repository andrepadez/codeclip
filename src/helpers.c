#include "helpers.h"
#include <string.h>

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

