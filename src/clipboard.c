#include "clipboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CMD 4096

int copy_to_clipboard(const char *outfile_path, const char *configured_tool) {
    if (!outfile_path || !*outfile_path) return -1;

    int copied = 0;

    // 1. Configured clipboard tool
    if (configured_tool && *configured_tool) {
        char cmd[MAX_CMD];
        snprintf(cmd, sizeof(cmd), "%s < '%s' 2>/dev/null", configured_tool, outfile_path);
        int ret = system(cmd);
        if (ret == 0) {
            printf("📋 Code copied using configured tool: %s\n", configured_tool);
            return 0;
        } else {
            fprintf(stderr,
                    "⚠️  Failed to run configured clipboard tool: \"%s\" — trying auto-detect...\n",
                    configured_tool);
        }
    }

    // 2. Auto-detect fallback
    const char *tools[] = {
        "wl-copy",
        "xclip -selection clipboard",
        "pbcopy"
    };

    for (int i = 0; i < 3; i++) {
        char cmd[MAX_CMD];
        snprintf(cmd, sizeof(cmd), "%s < '%s' 2>/dev/null", tools[i], outfile_path);
        if (system(cmd) == 0) {
            printf("📋 Code copied using %s\n", tools[i]);
            copied = 1;
            break;
        }
    }

    if (!copied) {
        fprintf(stderr, "⚠️  No clipboard tool found (tried wl-copy, xclip, pbcopy)\n");
        return -1;
    }

    return 0;
}

