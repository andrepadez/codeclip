#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stddef.h>

#define MAX_PATH 4096

struct Config {
    char output_dir[MAX_PATH];
    char clipboard_tool[64];
};

int load_config(struct Config *cfg);
int ensure_config_dir(void);

#endif

