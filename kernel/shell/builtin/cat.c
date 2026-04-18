#include "builtin.h"
#include "fs/vfs.h"
#include "drivers/fbcon.h"
#include "libk/string.h"

void sh_resolve_path(const char *in, char *out, size_t outsz)
{
    if (in[0] == '/') {
        // absolute path
        strncpy(out, in, outsz);
        out[outsz - 1] = '\0';
        return;
    }

    // relative path: cwd + "/" + in
    strncpy(out, cwd, outsz);
    out[outsz - 1] = '\0';

    size_t len = strlen(out);
    if (len + 1 < outsz) {
        out[len] = '/';
        out[len + 1] = '\0';
    }

    strncat(out, in, outsz - strlen(out)-1);
}

int builtin_cat(int *args, char **argv)
{
    while (*args == ' ') args++;
    if (*args == '\0') {
        printf("cat: missing path\n");
        return;
    }

    char path[128];
    sh_resolve_path(args, path, sizeof(path));

    struct file *f;
    if (vfs_open(path, VFS_O_RDONLY, &f) != 0) {
        printf("cat: cannot open %s\n", path);
        return;
    }

    char buf[128];
    usize n;

    while (vfs_read(f, buf, sizeof(buf) - 1, &n) == 0 && n > 0) {
        buf[n] = 0;
        printf("%s", buf);
    }

    printf("\n");
    vfs_close(f);
}
