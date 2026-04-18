#include "drivers/keyboard.h"
#include "drivers/tty.h"
#include "drivers/input.h"
#include "libk/string.h"
#include "libk/printf.h"

#define HISTORY_MAX 16
#define LINE_MAX 128

static char history[HISTORY_MAX][LINE_MAX];
static int history_count = 0; // how many entries are valid
static int history_pos = -1;  // current position when browsing

static void add_history(const char *line)
{
    if (line[0] == '\0')
        return;

    if (history_count < HISTORY_MAX)
    {
        strcpy(history[history_count++], line);
    }
    else
    {
        // shift up, drop oldest
        for (int i = 1; i < HISTORY_MAX; i++)
            strcpy(history[i - 1], history[i]);
        strcpy(history[HISTORY_MAX - 1], line);
    }
}

/* redraw prompt + buffer, and clear any leftover chars on the line */
static void redraw_line(const char *prompt, const char *buf, int prev_len)
{
    int len = (int)strlen(buf);

    /* return to line start */
    TTY_putc('\r');

    /* print prompt + buffer */
    printf("%s%s", prompt, buf);

    /* if previous content was longer, overwrite the tail with spaces */
    if (prev_len > len)
    {
        int diff = prev_len - len;
        for (int i = 0; i < diff; i++)
            TTY_putc(' ');
        /* return again and reprint prompt+buf so cursor ends at end of buf */
        TTY_putc('\r');
        printf("%s%s", prompt, buf);
    }
}

int readline(const char *prompt, char *buf, int max)
{
    int len = 0;            // current line length
    int cursor = 0;         // cursor index in buf
    int last_drawn_len = 0; // length of buffer last time we redrew

    buf[0] = '\0';

    /* print initial prompt */
    printf("%s", prompt);

    /* start browsing at "one past last" */
    history_pos = history_count;

    for (;;)
    {
        int c;
        while ((c = kbd_try_getchar()) == -1)
        {
        }

        /* ---------- ARROW KEYS ---------- */

        if (c == KBD_ARROW_LEFT)
        {
            if (cursor > 0)
            {
                cursor--;
                TTY_putc('\b');
            }
            continue;
        }

        if (c == KBD_ARROW_RIGHT)
        {
            if (cursor < len)
            {
                TTY_putc(buf[cursor]);
                cursor++;
            }
            continue;
        }

        if (c == KBD_ARROW_UP)
        {
            if (history_count > 0 && history_pos > 0)
            {
                history_pos--;
                strncpy(buf, history[history_pos], max - 1);
                buf[max - 1] = '\0';
                len = cursor = (int)strlen(buf);

                redraw_line(prompt, buf, last_drawn_len);
                last_drawn_len = len;
            }
            continue;
        }

        if (c == KBD_ARROW_DOWN)
        {
            if (history_pos < history_count - 1)
            {
                history_pos++;
                strncpy(buf, history[history_pos], max - 1);
                buf[max - 1] = '\0';
                len = cursor = (int)strlen(buf);
            }
            else
            {
                history_pos = history_count;
                buf[0] = '\0';
                len = cursor = 0;
            }

            redraw_line(prompt, buf, last_drawn_len);
            last_drawn_len = len;
            continue;
        }

        /* ---------- ENTER ---------- */

        if (c == '\n' || c == '\r')
        {
            TTY_putc('\n');
            buf[len] = '\0';
            add_history(buf);
            return len;
        }

        /* ---------- BACKSPACE ---------- */

        if (c == '\b')
        {
            if (cursor > 0)
            {
                /* shift left */
                memmove(&buf[cursor - 1], &buf[cursor], len - cursor);
                len--;
                cursor--;

                buf[len] = '\0';

                redraw_line(prompt, buf, last_drawn_len);
                last_drawn_len = len;

                /* move cursor to correct spot */
                int to_move = len - cursor;
                for (int i = 0; i < to_move; i++)
                    TTY_putc('\b');
            }
            continue;
        }

        /* ---------- PRINTABLE CHAR ---------- */

        if (c >= 32 && c < 127 && len < max - 1)
        {
            /* shift right to make room */
            memmove(&buf[cursor + 1], &buf[cursor], len - cursor);
            buf[cursor] = (char)c;
            len++;
            cursor++;

            buf[len] = '\0';

            redraw_line(prompt, buf, last_drawn_len);
            last_drawn_len = len;

            /* move cursor back to correct spot */
            int to_move = len - cursor;
            for (int i = 0; i < to_move; i++)
                TTY_putc('\b');
        }
    }
}
