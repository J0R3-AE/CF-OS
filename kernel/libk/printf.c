#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "arch/io.h"
#include <libk/printf.h>

#include "drivers/tty.h"

static void out_char(char **buf, usize *remaining, char c)
{
    if (*remaining > 1)
    {
        **buf = c;
        (*buf)++;
        (*remaining)--;
    }
}

static void out_str(char **buf, usize *remaining, const char *s)
{
    while (*s)
    {
        out_char(buf, remaining, *s++);
    }
}

static void out_uint(char **buf, usize *remaining, unsigned int v, int base)
{
    char tmp[32];
    int i = 0;

    if (v == 0)
    {
        out_char(buf, remaining, '0');
        return;
    }

    while (v && i < (int)sizeof(tmp))
    {
        unsigned int d = v % base;
        tmp[i++] = (d < 10) ? ('0' + d) : ('a' + (d - 10));
        v /= base;
    }

    while (i--)
    {
        out_char(buf, remaining, tmp[i]);
    }
}

int vsnprintf(char *buf, usize size, const char *fmt, va_list ap)
{
    char *out = buf;
    usize remaining = size;

    while (*fmt)
    {
        if (*fmt != '%')
        {
            out_char(&out, &remaining, *fmt++);
            continue;
        }

        fmt++; // skip '%'

        switch (*fmt)
        {
        case 's':
        {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            out_str(&out, &remaining, s);
            break;
        }
        case 'c':
        {
            char c = (char)va_arg(ap, int);
            out_char(&out, &remaining, c);
            break;
        }
        case 'd':
        {
            int v = va_arg(ap, int);
            if (v < 0)
            {
                out_char(&out, &remaining, '-');
                v = -v;
            }
            out_uint(&out, &remaining, (unsigned)v, 10);
            break;
        }
        case 'u':
        {
            unsigned v = va_arg(ap, unsigned);
            out_uint(&out, &remaining, v, 10);
            break;
        }
        case 'x':
        {
            unsigned v = va_arg(ap, unsigned);
            out_uint(&out, &remaining, v, 16);
            break;
        }
        case 'p':
        {
            uintptr_t v = (uintptr_t)va_arg(ap, void *);
            out_str(&out, &remaining, "0x");
            out_uint(&out, &remaining, (unsigned)v, 16);
            break;
        }
        case '%':
            out_char(&out, &remaining, '%');
            break;

        default:
            out_char(&out, &remaining, '?');
            break;
        }

        fmt++;
    }

    if (remaining > 0)
        *out = '\0';
    else
        buf[size - 1] = '\0';

    return (int)(out - buf);
}

void vprintf(const char *fmt, va_list ap)
{
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    // x86SERIAL_writestr(buf);
    TTY_puts(buf);
}

void printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    // x86SERIAL_writestr(buf);
    TTY_puts(buf);
}

void panic(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    i386io_panic(buf);
}

void hexdump(const void *data, usize size)
{
    const u8 *bytes = (const u8 *)data;
    for (usize i = 0; i < size; i += 16)
    {
        printf("%08x  ", (unsigned)(uintptr_t)(bytes + i));
        for (usize j = 0; j < 16 && i + j < size; ++j)
        {
            printf("%02x ", bytes[i + j]);
        }
        printf("\n");
    }
}

void assert_fail(const char *expr, const char *file, int line)
{
    panic("Assertion failed: %s, at %s:%d", expr, file, line);
}