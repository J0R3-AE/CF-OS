#include <stdint.h>
#include <stddef.h>

#include "kernel/drivers/line.h"
#include "kernel/drivers/keyevent.h"
#include "kernel/drivers/tty.h"

#include "kernel/sched/thread.h"
#include "kernel/sched/scheduler.h"

#include "kernel/signal/signal.h"

#include "libc/mem.h"

#define LINE_BUFFER_SIZE 256

static char line_buffer[LINE_BUFFER_SIZE];
static size_t line_len = 0;

static volatile int g_line_ready = 0;
static volatile int g_line_eof = 0;


/* -------------------------------------------------------------------------- */
/* Reset                                                                      */
/* -------------------------------------------------------------------------- */

void line_reset(void)
{
    line_len = 0;
    g_line_ready = 0;
    g_line_eof = 0;
    line_buffer[0] = '\0';
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

    signal_add(
        &thread->signals,
        sig,
        &info);
}


/* -------------------------------------------------------------------------- */
/* Key event                                                                  */
/* -------------------------------------------------------------------------- */

int line_handle_keyevent(const key_event_t *ev)
{
    if (!ev)
        return 0;

    if (ev->type != KEY_EVENT_PRESS)
        return 0;

    uint8_t ch = ev->ascii;

    /*
     * Ctrl+C
     */
    if (ch == 0x03)
    {
        line_len = 0;
        line_buffer[0] = '\0';

        TTY_putc('^');
        TTY_putc('C');
        TTY_putc('\n');

        line_send_signal(SIGINT);

        return 0;
    }

    /*
     * Ctrl+Z
     */
    if (ch == 0x1A)
    {
        TTY_putc('^');
        TTY_putc('Z');
        TTY_putc('\n');

        line_send_signal(SIGTSTP);

        return 0;
    }

    /*
     * Ctrl+D = EOF.
     *
     * If the current line is empty, expose EOF.
     * Otherwise leave the current line alone.
     */
    if (ch == 0x04)
    {
        if (line_len == 0)
        {
            g_line_eof = 1;
            return 1;
        }

        return 0;
    }

    /*
     * Enter completes the current line.
     */
    if (ch == '\n')
    {
        if (line_len >= LINE_BUFFER_SIZE)
            line_len = LINE_BUFFER_SIZE - 1;

        line_buffer[line_len] = '\0';

        TTY_putc('\n');

        g_line_ready = 1;

        return 1;
    }

    /*
     * Backspace.
     */
    if (ch == '\b')
    {
        if (line_len > 0)
        {
            line_len--;

            line_buffer[line_len] = '\0';

            /*
             * Erase visually:
             *
             *   character
             *   ↓
             *   backspace
             *   space
             *   backspace
             */
            TTY_putc('\b');
            TTY_putc(' ');
            TTY_putc('\b');
        }

        return 0;
    }

    /*
     * Ignore other control characters.
     */
    if (ch < 32)
        return 0;

    /*
     * Add ordinary character.
     */
    if (line_len < LINE_BUFFER_SIZE - 1)
    {
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


void line_consume(void)
{
    g_line_ready = 0;
    g_line_eof = 0;
}