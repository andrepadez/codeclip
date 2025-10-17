#ifndef BFS_TRAVERSAL_H
#define BFS_TRAVERSAL_H

#include <sys/stat.h>
#include <ftw.h>
#include "helpers.h"

int bfs_traverse(const char *root,
                 int (*callback)(const char *, const struct stat *, int, struct FTW *));

#endif
