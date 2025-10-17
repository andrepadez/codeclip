#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>
#include <sys/stat.h>
#include <ftw.h>

#define MAX_PATH 4096

extern char target_dir[MAX_PATH];
extern FILE *outfile;

int should_skip(const char *path);
const char *lang_for(const char *path);
int process_file(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

#endif

