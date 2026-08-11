#include <syscall.h>

int ksys_print(const char *s)
{
    if (!s)
        return ERR_INVALID_ARGUMENT;

    while (*s)
    {
        TTY_putc(*s);
        serial_write_char(*s);
        s++;
    }
    return ERR_SUCCESS;
}