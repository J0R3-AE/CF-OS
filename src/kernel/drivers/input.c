#include "drivers/scancode.h"
#include "drivers/tty.h"
#include "drivers/input.h"
#include "libk/string.h"
#include "libk/mem.h"
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
    int len = 0;
    int cursor = 0;
    int last_drawn_len = 0;

    buf[0] = '\0';

    printf("%s", prompt);

    history_pos = history_count;

    for (;;)
    {
        int c = kbd_read();   // blocking read

        /* special key? */
        if (c & 0x80)
        {
            int key = c & 0x7F;

            if (key == KEY_LEFT)
            {
                if (cursor > 0)
                {
                    cursor--;
                    TTY_putc('\b');
                }
                continue;
            }

            if (key == KEY_RIGHT)
            {
                if (cursor < len)
                {
                    TTY_putc(buf[cursor]);
                    cursor++;
                }
                continue;
            }

            if (key == KEY_UP)
            {
                if (history_count > 0 && history_pos > 0)
                {
                    history_pos--;
                    strncpy(buf, history[history_pos], max - 1);
                    buf[max - 1] = '\0';
                    len = cursor = strlen(buf);

                    redraw_line(prompt, buf, last_drawn_len);
                    last_drawn_len = len;
                }
                continue;
            }

            if (key == KEY_DOWN)
            {
                if (history_pos < history_count - 1)
                {
                    history_pos++;
                    strncpy(buf, history[history_pos], max - 1);
                    buf[max - 1] = '\0';
                    len = cursor = strlen(buf);
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

            continue;
        }

        /* ENTER */
        if (c == '\n')
        {
            TTY_putc('\n');
            buf[len] = '\0';
            add_history(buf);
            return len;
        }

        /* BACKSPACE */
        if (c == '\b')
        {
            if (cursor > 0)
            {
                memmove(&buf[cursor - 1], &buf[cursor], len - cursor);
                len--;
                cursor--;
                buf[len] = '\0';

                redraw_line(prompt, buf, last_drawn_len);
                last_drawn_len = len;

                int to_move = len - cursor;
                while (to_move--)
                    TTY_putc('\b');
            }
            continue;
        }

        /* printable */
        if (c >= 32 && c < 127 && len < max - 1)
        {
            memmove(&buf[cursor + 1], &buf[cursor], len - cursor);
            buf[cursor] = c;
            len++;
            cursor++;

            buf[len] = '\0';

            redraw_line(prompt, buf, last_drawn_len);
            last_drawn_len = len;

            int to_move = len - cursor;
            while (to_move--)
                TTY_putc('\b');
        }
    }
}
