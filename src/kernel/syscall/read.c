#include <syscall.h>

#include "kernel/drivers/tty.h"
#include "kernel/sched/scheduler.h"

int ksys_read(int fd, void *buf, usize count)
{
    if (!buf || count == 0)
        return ERR_INVALID_ARGUMENT;

    /* stdin */
    if (fd == 0)
    {
        char *out = (char *)buf;

        for (;;)
        {
            if (TTY_input_eof())
            {
                TTY_input_consume();
                return 0;
            }

            if (TTY_input_ready())
            {
                usize n =
                    TTY_readline(out, count);

                TTY_input_consume();

                return (int)n;
            }

            /*
             * No completed line yet.
             * Let other kernel threads run.
             */
            sched_yield();
        }
    }

    fd_table_t *fdt =
        syscall_get_fd_table();

    struct file *f =
        fdt ? fd_get(fdt, fd) : NULL;

    if (!f)
        return ERR_BAD_FILE_DESCRIPTOR;

    usize nread = 0;

    int r =
        vfs_read(
            f,
            buf,
            count,
            &nread);

    return (r == 0)
        ? (int)nread
        : r;
}