#include <stdint.h>
#include <stddef.h>

#include "kernel/drivers/line.h"
#include "kernel/drivers/keyevent.h"
#include "kernel/drivers/tty.h"

#include "kernel/sched/thread.h"
#include "kernel/sched/scheduler.h"

#include "kernel/signal/signal.h"
#include "libc/mem.h"
#include "libc/string.h"

#define LINE_BUFFER_SIZE 256
#define LINE_HISTORY_SIZE 8

static char line_buffer[LINE_BUFFER_SIZE];
static size_t line_len = 0;
static char line_history[LINE_HISTORY_SIZE][LINE_BUFFER_SIZE];
static char line_draft[LINE_BUFFER_SIZE];
static size_t line_history_count = 0;
static int line_history_index = -1;

static volatile int g_line_ready = 0;
static volatile int g_line_eof = 0;

/* -------------------------------------------------------------------------- */
/* Reset                                                                      */
/* -------------------------------------------------------------------------- */

void line_reset(void)
{
    line_len = 0;
    line_buffer[0] = '\0';
    line_history_index = -1;

    g_line_ready = 0;
    g_line_eof = 0;
}

static void line_store_history(void)
{
    if (!line_len)
        return;

    if (line_history_count &&
        strcmp(line_history[line_history_count - 1], line_buffer) == 0)
        return;

    if (line_history_count == LINE_HISTORY_SIZE)
    {
        for (size_t i = 1; i < LINE_HISTORY_SIZE; i++)
            memcpy(line_history[i - 1], line_history[i], LINE_BUFFER_SIZE);
        line_history_count--;
    }

    memcpy(line_history[line_history_count++], line_buffer, line_len + 1);
}

static void line_replace(const char *text)
{
    for (size_t i = 0; i < line_len; i++)
    {
        TTY_putc('\b');
        TTY_putc(' ');
        TTY_putc('\b');
    }

    line_len = strlen(text);
    memcpy(line_buffer, text, line_len + 1);
    for (size_t i = 0; i < line_len; i++)
        TTY_putc(line_buffer[i]);
}

static void line_history_move(int direction)
{
    if (!line_history_count)
        return;

    if (direction < 0)
    {
        if (line_history_index < 0)
        {
            memcpy(line_draft, line_buffer, line_len + 1);
            line_history_index = (int)line_history_count - 1;
        }
        else if (line_history_index > 0)
            line_history_index--;
        else
            return;

        line_replace(line_history[line_history_index]);
        return;
    }

    if (line_history_index < 0)
        return;

    if (line_history_index < (int)line_history_count - 1)
        line_replace(line_history[++line_history_index]);
    else
    {
        line_history_index = -1;
        line_replace(line_draft);
    }
}

/* -------------------------------------------------------------------------- */
/* Signal helper                                                              */
/* -------------------------------------------------------------------------- */

static void line_send_signal(int sig)
{
    thread_t *thread = sched_current();

    if (!thread)
        return;

    siginfo_t info;
    memset(&info, 0, sizeof(info));

    info.si_signo = sig;
    info.si_code = 0;

    signal_add(&thread->signals, sig, &info);
}

/* -------------------------------------------------------------------------- */
/* Key event                                                                  */
/* -------------------------------------------------------------------------- */

/*
 * Returns 1 when a complete line or EOF is available.
 * Returns 0 when more input is needed.
 */

int line_handle_keyevent(const key_event_t *ev)
{
    if (!ev)
        return 0;

    if (ev->type != KEY_EVENT_PRESS)
        return 0;

    /* F12 is reserved for the window manager. */
    if (ev->keycode == KEY_F12)
        return 0;

    /*
     * Don't modify input after the current line has been submitted.
     */
    if (g_line_ready || g_line_eof)
        return 0;

    if (ev->keycode == KEY_UP)
    {
        line_history_move(-1);
        return 0;
    }
    if (ev->keycode == KEY_DOWN)
    {
        line_history_move(1);
        return 0;
    }
    if (ev->keycode == KEY_PAGE_UP)
    {
        TTY_scrollback(-(int)(TTY_ROWS > 1 ? TTY_ROWS - 1 : 1));
        return 0;
    }
    if (ev->keycode == KEY_PAGE_DOWN)
    {
        TTY_scrollback((int)(TTY_ROWS > 1 ? TTY_ROWS - 1 : 1));
        return 0;
    }

    uint8_t ch = ev->ascii;

    /* Ctrl+C: interrupt the current operation and clear the input. */
    if (ch == 0x03)
    {
        line_len = 0;
        line_buffer[0] = '\0';
        line_history_index = -1;

        TTY_putc('^');
        TTY_putc('C');
        TTY_putc('\n');

        line_send_signal(SIGINT);
        return 0;
    }

    /* Ctrl+Z: request suspension. */
    if (ch == 0x1A)
    {
        TTY_putc('^');
        TTY_putc('Z');
        TTY_putc('\n');

        line_send_signal(SIGTSTP);
        return 0;
    }

    /*
     * Ctrl+D:
     * - Empty buffer: report EOF.
     * - Non-empty buffer: submit the current input.
     */
    if (ch == 0x04)
    {
        if (line_len == 0)
        {
            g_line_eof = 1;
            sched_yield();
            return 1;
        }

        line_buffer[line_len] = '\0';
        line_store_history();
        g_line_ready = 1;

        TTY_putc('\n');
        return 1;
    }

    /* Enter: submit the current line. */
     if (ch == '\n' || ch == '\r')
    {
        line_buffer[line_len] = '\0';
        line_store_history();
        line_history_index = -1;

        g_line_ready = 1;

        TTY_putc('\n');

        /* Give the waiting reader a chance to resume. */
        sched_yield();

        return 1;
    }

    /* Backspace: support both common ASCII encodings. */
    if (ch == '\b' || ch == 0x7F)
    {
        if (line_len > 0)
        {
            line_history_index = -1;
            line_len--;
            line_buffer[line_len] = '\0';

            TTY_putc('\b');
            TTY_putc(' ');
            TTY_putc('\b');
        }

        return 0;
    }

    /* Ignore control characters. */
    if (ch < 32)
        return 0;

    /* Add and echo ordinary characters. */
    if (line_len < LINE_BUFFER_SIZE - 1)
    {
        line_history_index = -1;
        line_buffer[line_len++] = (char)ch;
        line_buffer[line_len] = '\0';

        TTY_putc((char)ch);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Input state                                                                */
/* -------------------------------------------------------------------------- */

int line_ready(void)
{
    return g_line_ready;
}

int line_eof(void)
{
    return g_line_eof;
}

/* -------------------------------------------------------------------------- */
/* Read completed line                                                        */
/* -------------------------------------------------------------------------- */

const char *line_get_buffer(void)
{
    return line_buffer;
}

size_t line_get_length(void)
{
    return line_len;
}

/* -------------------------------------------------------------------------- */
/* Consume input                                                              */
/* -------------------------------------------------------------------------- */

void line_consume(void)
{
    line_reset();
}

void line_handle_ascii(u8 character)
{
    key_event_t event = {
        .type = KEY_EVENT_PRESS,
        .keycode = KEY_NONE,
        .ascii = character,
    };
    line_handle_keyevent(&event);
}

int line_read(char *buffer, usize capacity, bool wait)
{
    if (!buffer || capacity == 0)
        return -1;

    while (!line_ready() && !line_eof())
    {
        if (!wait)
            return 0;
        sched_yield();
    }

    if (line_eof())
    {
        line_consume();
        return 0;
    }

    usize length = line_get_length();
    if (length >= capacity)
        return -1;
    memcpy(buffer, line_get_buffer(), length);
    buffer[length] = '\0';
    line_consume();
    return (int)length;
}

int line_scan(char *buffer, usize capacity)
{
    return line_read(buffer, capacity, true);
}