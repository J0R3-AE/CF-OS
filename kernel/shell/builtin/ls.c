#include "builtin.h"
#include "fs/vfs.h"
#include "drivers/fbcon.h"
#include "libk/string.h"
#include "fs/fs_types.h"

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

int builtin_ls(const char *args, char **argv)
{
    char path[128];

    while (*args == ' ') args++;

    if (*args == '\0') {
        /* no arg: list current directory */
        strncpy(path, cwd, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    } else {
        sh_resolve_path(args, path, sizeof(path));
    }

    struct vnode *vn;
    if (vfs_lookup(path, &vn) != 0) {
        printf("ls: cannot access %s\n", path);
        return;
    }

    if (!vn->ops || !vn->ops->readdir) {
        printf("ls: not a directory: %s\n", path);
        return;
    }

    const char *name;
    vnode_type_t type;

    for (usize i = 0;; i++) {
        if (vn->ops->readdir(vn, i, &name, &type) != 0)
            break;

        printf("%s%s\n", name, type == VNODE_TYPE_DIR ? "/" : "");
    }
}
