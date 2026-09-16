#include <syscall.h>
#include "kernel/mm/heap.h"
#include "libc/string.h"

int ksys_open(const char *path, int flags)
{
    if (!path)
        return ERR_INVALID_ARGUMENT;

    const char *use_path = path;
    char *tmp_path = NULL;

    /* Resolve relative paths against process cwd */
    if (path[0] != '/')
    {
        process_t *p = proc_current();
        if (!p)
            return ERR_INTERNAL;

        size_t cwd_len = strlen(p->cwd);
        size_t path_len = strlen(path);
        size_t need = cwd_len + 1 + path_len + 1;
        tmp_path = malloc(need);
        if (!tmp_path)
            return ERR_INTERNAL;

        if (strcmp(p->cwd, "/") == 0)
        {
            tmp_path[0] = '/';
            memcpy(tmp_path + 1, path, path_len + 1);
        }
        else
        {
            memcpy(tmp_path, p->cwd, cwd_len);
            tmp_path[cwd_len] = '/';
            memcpy(tmp_path + cwd_len + 1, path, path_len + 1);
        }

        use_path = tmp_path;
    }

    struct file *f = NULL;
    int r = vfs_open(use_path, (u32)flags, &f);

    if (tmp_path)
        free(tmp_path);

    if (r < 0 || !f)
    {
        return ERR_NOT_FOUND;
    }

    fd_table_t *fdt = syscall_get_fd_table();
    if (!fdt)
    {
        file_unref(f);
        return ERR_INTERNAL;
    }

    int fd = fd_alloc(fdt, f);
    if (fd < 0)
    {
        file_unref(f);
        return ERR_TOO_MANY_OPEN_FILES;
    }

    return fd;
}