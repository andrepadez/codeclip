#include "config_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

int ensure_config_dir(void) {
    const char *home = getenv("HOME");
    if (!home) return -1;

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/.config/.codeclip", home);
    return mkdir(path, 0755) == 0 || access(path, F_OK) == 0;
}

int load_config(struct Config *cfg) {
    if (!cfg) return -1;

    const char *home = getenv("HOME");
    if (!home) return -1;

    ensure_config_dir();

    char config_path[MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s/.config/.codeclip/config.toml", home);

    // defaults
    snprintf(cfg->output_dir, sizeof(cfg->output_dir), "%s/codeclips", home);
    cfg->clipboard_tool[0] = '\0';

    FILE *f = fopen(config_path, "r");
    if (!f) return 0; // no config, just use defaults

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // skip comments and empty lines
        if (line[0] == '#' || isspace(line[0])) continue;

        char key[64], value[448];
        if (sscanf(line, " %63[^=]= \"%447[^\"]\"", key, value) == 2 ||
            sscanf(line, " %63[^=]= %447s", key, value) == 2) {

            if (strcmp(key, "output_dir") == 0) {
                if (value[0] == '~')
                    snprintf(cfg->output_dir, sizeof(cfg->output_dir), "%s%s", home, value + 1);
                else
                    strncpy(cfg->output_dir, value, sizeof(cfg->output_dir));
            } else if (strcmp(key, "clipboard_tool") == 0) {
                strncpy(cfg->clipboard_tool, value, sizeof(cfg->clipboard_tool) - 1);
                cfg->clipboard_tool[sizeof(cfg->clipboard_tool) - 1] = '\0';
            }
        }
    }
    fclose(f);
    return 1;
}

