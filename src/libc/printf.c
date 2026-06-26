#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "libk/printf.h"
#include "drivers/tty.h"
#include "arch/io.h"

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
    if (!s) s = "(null)";
    while (*s)
        out_char(buf, remaining, *s++);
}

static void out_uint(char **buf, usize *remaining, uint32_t v, uint32_t base)
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
        uint32_t d = v % base;
        tmp[i++] = (d < 10) ? ('0' + d) : ('a' + (d - 10));
        v /= base;
    }

    while (i--)
        out_char(buf, remaining, tmp[i]);
}

static void out_int(char **buf, usize *remaining, int v)
{
    if (v < 0)
    {
        out_char(buf, remaining, '-');
        out_uint(buf, remaining, (uint32_t)(-(int64_t)v), 10);
    }
    else
    {
        out_uint(buf, remaining, (uint32_t)v, 10);
    }
}

int snprintf(char *buf, usize size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int ret = vsnprintf(buf, size, fmt, ap);

    va_end(ap);
    return ret;
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
            out_str(&out, &remaining, va_arg(ap, const char *));
            break;

        case 'c':
            out_char(&out, &remaining, (char)va_arg(ap, int));
            break;

        case 'd':
            out_int(&out, &remaining, va_arg(ap, int));
            break;

        case 'u':
            out_uint(&out, &remaining, va_arg(ap, uint32_t), 10);
            break;

        case 'x':
            out_uint(&out, &remaining, va_arg(ap, uint32_t), 16);
            break;

        case 'p':
        {
            uintptr_t p = (uintptr_t)va_arg(ap, void *);
            out_str(&out, &remaining, "0x");
            out_uint(&out, &remaining, (uint32_t)p, 16);
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

    if (size > 0)
        *out = '\0';

    return (int)(out - buf);
}

void vprintf(const char *fmt, va_list ap)
{
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    TTY_puts(buf);
}

void printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    serial_write(buf);
    TTY_puts(buf);
}

void panic(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    serial_write(buf);
    TTY_puts(buf);
}

void hexdump(const void *data, usize size)
{
    const u8 *bytes = (const u8 *)data;

    for (usize i = 0; i < size; i += 16)
    {
        printf("%p  ", (void *)(bytes + i));

        for (usize j = 0; j < 16 && i + j < size; j++)
            printf("%x ", bytes[i + j]);

        printf("\n");
    }
}

void assert_fail(const char *expr, const char *file, int line)
{
    panic("Assertion failed: %s at %s:%d", expr, file, line);
}