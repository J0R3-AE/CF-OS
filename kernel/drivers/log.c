#include "libk/log.h"
#include "arch/io.h"
#include "drivers/tty.h"
#include "libk/printf.h"
#include <stdarg.h>
#include <stddef.h>

static log_level_t current_level = LOG_DEBUG;

static const char *level_tag(log_level_t lvl) {
    switch (lvl) {
    case LOG_DEBUG: 
    TTY_setcolor(TTY_COLOR_CYAN, TTY_COLOR_BLACK);
    return "DEBUG"; 
    case LOG_INFO:  
    TTY_setcolor(TTY_COLOR_LIGHT_GREEN, TTY_COLOR_BLACK);
    return "INFO"; 
    case LOG_WARN:  
    TTY_setcolor(TTY_COLOR_YELLOW, TTY_COLOR_BLACK);
    return "WARN"; 
    case LOG_ERROR: 
    TTY_setcolor(TTY_COLOR_LIGHT_RED, TTY_COLOR_BLACK);
    return "ERROR";
    case LOG_FATAL: 
    TTY_setcolor(TTY_COLOR_WHITE, TTY_COLOR_LIGHT_RED);
    return "FATAL";
    case LOG_LOG:
    TTY_setcolor(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    return "LOG";
    }
    return "????";
}

void log_set_level(log_level_t lvl) {
    current_level = lvl;
}

void log_vwrite(log_level_t lvl, const char *fmt, va_list ap)
{
    if (lvl < current_level) return;

    char buf[256];
    /* your own vsnprintf/kvprintf here */
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    if (n < 0) return;

    /* prefix with level */
    i386SERIAL_writestr("[");
    i386SERIAL_writestr(level_tag(lvl));
    i386SERIAL_writestr("] ");
    i386SERIAL_writestr(buf);
    i386SERIAL_writestr("\n");

    TTY_putc('[');  
    TTY_puts(level_tag(lvl));
    TTY_puts("] ");
    TTY_setcolor(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    TTY_puts(buf);
    TTY_putc('\n');
    TTY_setcolor(TTY_COLOR_WHITE, TTY_COLOR_BLACK);

    if (lvl == LOG_FATAL) {
        /* optional: halt */
        for (;;)
            __asm__ __volatile__("hlt");
    }
}

void log_write(log_level_t lvl, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vwrite(lvl, fmt, ap);
    va_end(ap);
}
