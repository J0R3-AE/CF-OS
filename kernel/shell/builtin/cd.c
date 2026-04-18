#include "builtin.h"
#include "fs/vfs.h"
#include "drivers/fbcon.h"
#include "libk/string.h"

void sh_resolve_path(const char *in, char *out, size_t outsz)
{
    /* Case 1: absolute path */
    if (in[0] == '/') {
        strncpy(out, in, outsz);
        out[outsz - 1] = '\0';
        return;
    }

    /* Case 2: go up one directory */
    if (strcmp(in, "..") == 0) {
        /* find last slash in cwd */
        char *slash = strrchr(cwd, '/');

        if (!slash || slash == cwd) {
            /* already at root */
            strcpy(out, "/");
        } else {
            size_t len = slash - cwd;
            strncpy(out, cwd, len);
            out[len] = '\0';
        }
        return;
    }

    /* Case 3: relative path (normal) */
    strncpy(out, cwd, outsz);
    out[outsz - 1] = '\0';

    size_t len = strlen(out);
    if (len + 1 < outsz) {
        out[len] = '/';
        out[len + 1] = '\0';
    }

    strncat(out, in, outsz - strlen(out) - 1);
}

int builtin_cd(int *args, char **argv)
{
    while (*args == ' ') args++;

    char path[128];
    if (*args == '\0') {
        /* no arg: go to root for now */
        strcpy(path, "/");
    } else {
        sh_resolve_path(args, path, sizeof(path));
    }

    struct vnode *vn;
    if (vfs_lookup(path, &vn) != 0) {
        printf("cd: no such directory: %s\n", path);
        return;
    }

    if (vn->type != VNODE_TYPE_DIR) {
        printf("cd: not a directory: %s\n", path);
        return;
    }

    strncpy(cwd, path, sizeof(cwd) - 1);
    cwd[sizeof(cwd) - 1] = '\0';
}
