#ifndef HELPERS_H

#define HELPERS_H

#include <stdio.h>
#include <sys/stat.h>
#include <ftw.h>
#include "config_manager.h"

#define MAX_PATH 4096
#define CODECLIP_VERSION "1.0.0"

extern char target_dir[MAX_PATH];
extern FILE *outfile;

int should_skip(const char *path);
const char *lang_for(const char *path);
int process_file(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
int find_git_root(const char *start_path, char *git_root, size_t size,
                  char *project_name, size_t name_size);

int init_config(struct Config *cfg);
void detect_project(const char *target);
void build_output_path(char *outfile_path, size_t size,
                       const char *base_dir, const char *target);
int write_single_file(const char *path, struct Config *cfg, int clipboard_mode);
int write_directory(const char *path, struct Config *cfg, int clipboard_mode);
void maybe_copy_to_clipboard(const char *outfile_path, struct Config *cfg, int clipboard_mode);

// for sub-commands
int clear_clips(const struct Config *cfg, int force);
void print_version(void);
void print_help(void);

#endif

