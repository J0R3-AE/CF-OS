#include "kernel/drivers/tty.h"
#include "libc/string.h"

static void cmd_help(void);
static void cmd_clear(void);

void shell_execute(const char *line)
{
    if (!strcmp(line, "help"))
    {
        cmd_help();
        return;
    }

    if (!strcmp(line, "clear"))
    {
        cmd_clear();
        return;
    }

    if (!strcmp(line, "echo"))
    {
        TTY_puts("Usage: echo <text>\n");
        return;
    }

    if (!strncmp(line, "echo ", 5))
    {
        TTY_puts(line + 5);
        TTY_putc('\n');
        return;
    }

    TTY_puts("Unknown command: ");
    TTY_puts(line);
    TTY_putc('\n');
}

static void cmd_help(void)
{
    TTY_puts("Commands:\n");
    TTY_puts("  help\n");
    TTY_puts("  clear\n");
    TTY_puts("  echo\n");
}

static void cmd_clear(void)
{
    TTY_clear();
}