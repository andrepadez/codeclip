#include "helpers.h"
#include <string.h>
#include <stdlib.h>

int process_file(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F && !should_skip(path)) {
        const char *rel = path + strlen(target_dir) + 1;
        const char *lang = lang_for(path);

        fprintf(outfile, "### %s\n\n```%s\n", rel, lang);

        FILE *f = fopen(path, "r");
        if (f) {
            char buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
                fwrite(buf, 1, n, outfile);
            fclose(f);
        }

        fprintf(outfile, "\n```\n\n");
    }
    return 0;
}

