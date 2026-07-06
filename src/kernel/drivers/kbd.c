#include "drivers/kbd.h"
#include "drivers/keyboard.h"
#include "drivers/serial.h"
#include "drivers/tty.h"
#include "sched/sched.h"
#include <stdbool.h>

#define KBD_BUF_SIZE 256 /* power of two */

static int kbd_buf[KBD_BUF_SIZE];
static volatile unsigned int kbd_head = 0;
static volatile unsigned int kbd_tail = 0;

/* ----------------------------
   ring buffer helpers
---------------------------- */

static inline unsigned int next_index(unsigned int i)
{
    return (i + 1u) & (KBD_BUF_SIZE - 1u);
}

void kbd_init(void)
{
    kbd_head = 0;
    kbd_tail = 0;
}

void kbd_flush(void)
{
    kbd_head = 0;
    kbd_tail = 0;
}

int kbd_has_char(void)
{
    return kbd_head != kbd_tail;
}

/* ----------------------------
   push from IRQ layer
   (IMPORTANT: already ASCII / KEY_* translated here)
---------------------------- */

void kbd_push(int key)
{
    if (key == KEY_NONE)
        return;

    unsigned int next = next_index(kbd_head);

    /* drop newest if full */
    if (next == kbd_tail)
        return;

    kbd_buf[kbd_head] = key;
    kbd_head = next;
}

/* ----------------------------
   non-blocking read
---------------------------- */

int kbd_try_getchar(void)
{
    if (kbd_head != kbd_tail)
    {
        int key = kbd_buf[kbd_tail];
        kbd_tail = next_index(kbd_tail);
        return key;
    }

    if (serial_is_initialized() && serial_received())
    {
        char c = serial_read_char();
        if (c == '\r')
            c = '\n';
        TTY_putc(c);
        return (unsigned char)c;
    }

    return -1;
}

/* ----------------------------
   blocking read (line input)
---------------------------- */

int kbd_read(void *buf, usize len)
{
    if (!buf || len == 0)
        return 0;

    char *out = (char *)buf;
    usize count = 0;

    while (count < len - 1)
    {
        int key = kbd_try_getchar();

        if (key < 0)
        {
            ksched_yield();
            continue;
        }

        /* ENTER */
        if (key == KEY_ENTER || key == '\n')
        {
            TTY_putc('\n');
            out[count++] = '\n';
            break;
        }

        /* BACKSPACE */
        if (key == KEY_BACKSPACE || key == '\b')
        {
            if (count > 0)
            {
                count--;

                /* Erase character on screen */
                TTY_putc('\b');
                TTY_putc(' ');
                TTY_putc('\b');
            }

            continue;
        }

        /* ESC */
        if (key == KEY_ESC)
        {
            out[count++] = 27;
            break;
        }

        /* Arrow keys (ignore for now) */
        if (key >= KEY_UP && key <= KEY_DOWN)
        {
            continue;
        }

        /* Printable ASCII */
        if (key >= 32 && key < 127)
        {
            TTY_putc((char)key);      /* Echo to screen */
            out[count++] = (char)key; /* Store in buffer */
        }
    }

    out[count] = '\0';
    return (int)count;
}