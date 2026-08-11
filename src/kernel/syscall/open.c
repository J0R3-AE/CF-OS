#include <syscall.h>

int ksys_open(const char *path, int flags)
{
    struct file *f = NULL;
    int r = vfs_open(path, (u32)flags, &f);
    
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

    return ERR_SUCCESS;
}