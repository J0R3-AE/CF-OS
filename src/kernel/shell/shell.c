#include "kernel/drivers/tty.h"
#include "kernel/drivers/line.h"
#include "libc/errno.h"
#include "libc/string.h"

#define SHELL_MAX_LINE 256

static char line[SHELL_MAX_LINE];

extern void shell_execute(const char *command);

void shell_init(void)
{
    TTY_puts("\n");
    TTY_puts("=====================================\n");
    TTY_puts("        AescOS Kernel Shell\n");
    TTY_puts("=====================================\n\n");
}

void shell_run(void)
{
    shell_init();
    while (1)
    {
        TTY_puts("> ");

        int len = line_scan(line, sizeof(line));

        if (len < 0)
        {
            if (len == ERR_BUFFER_TOO_SMALL)
                TTY_puts("Input line is too long.\n");
            else
                TTY_puts("Input failed; try again.\n");
            continue;
        }
        if (len == 0)
            continue;

        shell_execute(line);
    }
}