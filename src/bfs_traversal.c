#include "bfs_traversal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Simple dynamic queue of directory paths
typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} DirQueue;

static void queue_init(DirQueue *q) {
    q->items = NULL;
    q->count = 0;
    q->capacity = 0;
}

static void queue_push(DirQueue *q, const char *path) {
    if (q->count == q->capacity) {
        q->capacity = q->capacity ? q->capacity * 2 : 32;
        q->items = realloc(q->items, q->capacity * sizeof(char *));
    }
    q->items[q->count++] = strdup(path);
}

static char *queue_pop(DirQueue *q) {
    if (q->count == 0) return NULL;
    char *res = q->items[0];
    memmove(q->items, q->items + 1, (q->count - 1) * sizeof(char *));
    q->count--;
    return res;
}

static void queue_free(DirQueue *q) {
    for (size_t i = 0; i < q->count; i++)
        free(q->items[i]);
    free(q->items);
}

// Breadth-first traversal
// callback(path, sb) is called for each file (not directories)
int bfs_traverse(const char *root,
                 int (*callback)(const char *, const struct stat *, int, struct FTW *)) {
    struct stat st;
    if (stat(root, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a directory\n", root);
        return -1;
    }

    DirQueue q;
    queue_init(&q);
    queue_push(&q, root);

    struct FTW dummy_ftw; // minimal placeholder
    dummy_ftw.base = 0;
    dummy_ftw.level = 0;

    while (q.count > 0) {
        char *dirpath = queue_pop(&q);
        DIR *dir = opendir(dirpath);
        if (!dir) {
            perror("opendir");
            free(dirpath);
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char fullpath[MAX_PATH];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);

            if (lstat(fullpath, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    if (should_skip(fullpath))
                        continue;
                    queue_push(&q, fullpath);
                } else if (S_ISREG(st.st_mode)) {
                    callback(fullpath, &st, FTW_F, &dummy_ftw);
                }
            }
        }

        closedir(dir);
        free(dirpath);
    }

    queue_free(&q);
    return 0;
}
