#include <syscall.h>

int ksys_write(int fd, const void *buf, usize count)
{
    if (!buf || count == 0)
        return ERR_INVALID_ARGUMENT;

    /* Print fallback for stdout/stderr */
    if (fd == 1 || fd == 2)
    {
        const char *cbuf = (const char *)buf;
        usize written = 0;
        for (usize i = 0; i < count; i++)
        {
            TTY_putc(cbuf[i]);
            serial_write_char(cbuf[i]);
            written++;
        }
        return (int)written;
    }

    fd_table_t *fdt = syscall_get_fd_table();
    struct file *f = fdt ? fd_get(fdt, fd) : NULL;
    if (!f)
        return ERR_BAD_FILE_DESCRIPTOR;

    usize written = 0;
    int r = vfs_write(f, buf, count, &written);
    return (r == 0) ? (int)written : r;
}