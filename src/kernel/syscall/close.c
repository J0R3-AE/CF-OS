#include <syscall.h>

int ksys_close(int fd)
{
    fd_table_t *fdt = syscall_get_fd_table();
    if (!fdt)
        return ERR_INTERNAL;

    int r = fd_close(fdt, fd);
    return (r == 0) ? ERR_SUCCESS : ERR_BAD_FILE_DESCRIPTOR;
}