#include <syscall.h>

int ksys_read(int fd, void *buf, usize count)
{
    if (!buf || count == 0)
        return ERR_INVALID_ARGUMENT;

    /* stdin: cooked, non-blocking line read (partial line OK) */
    if (fd == 0)
    {
        /* If you have a line_get_buffer API that fills buf and returns bytes,
           call it here. Example:
               size_t n = line_get_buffer(buf, count);
               return (int)n;
        */
        return 0;
    }

    fd_table_t *fdt = syscall_get_fd_table();
    struct file *f = fdt ? fd_get(fdt, fd) : NULL;
    if (!f)
        return ERR_BAD_FILE_DESCRIPTOR;

    usize nread = 0;
    int r = vfs_read(f, buf, count, &nread);
    return (r == 0) ? (int)nread : r;
}