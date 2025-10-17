#include "config_manager.h"
#include "generated/config_default.h"
#include "generated/ignore_default.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>

static int mkdir_p(const char *path) {
    char tmp[MAX_PATH];
    strncpy(tmp, path, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = '\0';
    size_t len = strlen(tmp);
    if (len == 0) return -1;
    if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755);
}

// detect first available clipboard tool (wl-copy, xclip, pbcopy)
static const char *detect_clipboard_tool(void) {
    const char *tools[] = { "wl-copy", "xclip", "pbcopy" };
    for (int i = 0; i < 3; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", tools[i]);
        if (system(cmd) == 0) {
            return tools[i];
        }
    }
    return "";
}

int ensure_config_dir(void) {
    const char *home = getenv("HOME");
    if (!home || !*home) {
        struct passwd *pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : NULL;
    }

    if (!home) {
        fprintf(stderr, "Error: cannot determine home directory.\n");
        return -1;
    }

    char dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%s/.config/codeclip", home);

    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        if (mkdir_p(dir) != 0 && errno != EEXIST) {
            perror("mkdir_p");
            return -1;
        }
        printf("[codeclip] Created config directory: %s\n", dir);
    }
    return 0;
}

static void create_default_files(const char *config_path, const char *ignore_path, const char *home) {
    const char *detected_tool = detect_clipboard_tool();

    // --- Create config.yaml ---
    FILE *f = fopen(config_path, "w");
    if (f) {
        fprintf(f,
            "# Codeclip configuration file\n"
            "# Created automatically on first run\n\n"
            "output_dir: \"%s/.local/share/codeclips\"\n"
            "clipboard_tool: \"%s\" # detected automatically\n",
            home, detected_tool
        );
        fclose(f);
        printf("[codeclip] Created default config with clipboard tool: %s\n",
               detected_tool[0] ? detected_tool : "(none)");
    } else {
        perror("fopen config.yaml");
    }

    // --- Create codeclipignore from embedded default ---
    FILE *i = fopen(ignore_path, "w");
    if (i) {
        fputs(DEFAULT_IGNORE_FILE, i);
        fclose(i);
        printf("[codeclip] Created default ignore file: %s\n", ignore_path);
    } else {
        perror("fopen codeclipignore");
    }
}

int load_config(struct Config *cfg) {
    if (!cfg) return -1;

    const char *home = getenv("HOME");
    if (!home || !*home) {
        struct passwd *pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : NULL;
    }

    if (!home) {
        fprintf(stderr, "Error: cannot determine home directory.\n");
        return -1;
    }

    ensure_config_dir();

    char config_path[MAX_PATH];
    char ignore_path[MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s/.config/codeclip/config.yaml", home);
    snprintf(ignore_path, sizeof(ignore_path), "%s/.config/codeclip/codeclipignore", home);

    // defaults
    snprintf(cfg->output_dir, sizeof(cfg->output_dir), "%s/.local/share/codeclips", home);
    cfg->clipboard_tool[0] = '\0';

    int missing = 0;
    if (access(config_path, F_OK) != 0) missing = 1;
    if (access(ignore_path, F_OK) != 0) missing = 1;

    if (missing) {
        create_default_files(config_path, ignore_path, home);
    }

    FILE *f = fopen(config_path, "r");
    if (!f) return 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || isspace(line[0])) continue;

        char key[64], value[448];
        if (sscanf(line, " %63[^:]: \"%447[^\"]\"", key, value) == 2 ||
            sscanf(line, " %63[^:]: %447s", key, value) == 2) {

            if (strcmp(key, "output_dir") == 0) {
                if (value[0] == '~')
                    snprintf(cfg->output_dir, sizeof(cfg->output_dir), "%s%s", home, value + 1);
                else
                    strncpy(cfg->output_dir, value, sizeof(cfg->output_dir) - 1);
                cfg->output_dir[sizeof(cfg->output_dir) - 1] = '\0';
            } else if (strcmp(key, "clipboard_tool") == 0) {
                strncpy(cfg->clipboard_tool, value, sizeof(cfg->clipboard_tool) - 1);
                cfg->clipboard_tool[sizeof(cfg->clipboard_tool) - 1] = '\0';
            }
        }
    }
    fclose(f);
    return 1;
}
