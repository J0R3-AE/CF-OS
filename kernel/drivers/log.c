#include "libk/log.h"
#include "arch/io.h"
#include "drivers/tty.h"
#include "libk/printf.h"

#include <stdarg.h>
#include <stddef.h>

static log_level_t current_level = LOG_DEBUG;

static const char *level_tag(log_level_t lvl)
{
    switch (lvl)
    {
    case LOG_DEBUG: return "DEBUG";
    case LOG_INFO:  return "INFO";
    case LOG_OKAY:  return "OKAY";
    case LOG_WARN:  return "WARN";
    case LOG_ERROR: return "ERROR";
    case LOG_FATAL: return "FATAL";
    case LOG_LOG:   return "LOG";
    default:        return "????";
    }
}

static void level_color(log_level_t lvl, u8 *fg, u8 *bg)
{
    switch (lvl)
    {
    case LOG_DEBUG: *fg = TTY_COLOR_LIGHT_CYAN;         *bg = TTY_COLOR_BLACK;      break;
    case LOG_INFO:  *fg = TTY_COLOR_LIGHT_CYAN;         *bg = TTY_COLOR_BLACK;      break;
    case LOG_WARN:  *fg = TTY_COLOR_YELLOW;       *bg = TTY_COLOR_BLACK;      break;
    case LOG_ERROR: *fg = TTY_COLOR_LIGHT_RED;    *bg = TTY_COLOR_BLACK;      break;
    case LOG_FATAL: *fg = TTY_COLOR_WHITE;        *bg = TTY_COLOR_LIGHT_RED;  break;
    case LOG_LOG:   *fg = TTY_COLOR_WHITE;        *bg = TTY_COLOR_BLACK;      break;
    case LOG_OKAY:  *fg = TTY_COLOR_LIGHT_GREEN;  *bg = TTY_COLOR_BLACK;      break;
    default:        *fg = TTY_COLOR_WHITE;        *bg = TTY_COLOR_BLACK;      break;
    }
}

void log_set_level(log_level_t lvl)
{
    current_level = lvl;
}

void log_vwrite_ex(log_level_t lvl, const char *file, int line, const char *fmt, va_list ap)
{
    if (lvl < current_level)
        return;

    char msg[256];
    int n = vsnprintf(msg, sizeof(msg), fmt, ap);
    if (n < 0)
        return;

    char final[384];
    if (file && file[0])
        snprintf(final, sizeof(final), "[%s] %s:%d: %s", level_tag(lvl), file, line, msg);
    else
        snprintf(final, sizeof(final), "[%s] %s", level_tag(lvl), msg);

    /* serial */
    i386SERIAL_writestr(final);
    i386SERIAL_writestr("\n");

    /* tty */
    u8 fg, bg;
    level_color(lvl, &fg, &bg);
    TTY_setcolor(fg, bg);
    TTY_puts(final);
    TTY_putc('\n');
    TTY_setcolor(TTY_COLOR_WHITE, TTY_COLOR_BLACK);

    if (lvl == LOG_FATAL)
    {
        for (;;)
            __asm__ __volatile__("hlt");
    }
}

void log_write_ex(log_level_t lvl, const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vwrite_ex(lvl, file, line, fmt, ap);
    va_end(ap);
}

void log_vwrite(log_level_t lvl, const char *fmt, va_list ap)
{
    log_vwrite_ex(lvl, NULL, 0, fmt, ap);
}

void log_write(log_level_t lvl, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vwrite_ex(lvl, NULL, 0, fmt, ap);
    va_end(ap);
}