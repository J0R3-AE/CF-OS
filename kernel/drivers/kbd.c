#include "drivers/kbd.h"
#include "drivers/keyboard.h"
#include <stdbool.h>

#define KBD_BUF_SIZE 256   /* power of two */

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
    if (kbd_head == kbd_tail)
        return -1;

    int key = kbd_buf[kbd_tail];
    kbd_tail = next_index(kbd_tail);
    return key;
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
            asm volatile("hlt");
            continue;
        }

        /* ENTER */
        if (key == KEY_ENTER || key == '\n')
        {
            out[count++] = '\n';
            break;
        }

        /* BACKSPACE */
        if (key == KEY_BACKSPACE || key == '\b')
        {
            if (count > 0)
                count--;
            continue;
        }

        /* ESC (useful for menu exit) */
        if (key == KEY_ESC)
        {
            out[count++] = 27; /* ASCII ESC */
            break;
        }

        /* arrows go through as special codes */
        if (key >= KEY_UP && key <= KEY_DOWN)
        {
            out[count++] = key;
            continue;
        }

        /* normal ASCII */
        if (key >= 32 && key < 127)
        {
            out[count++] = (char)key;
        }

    }

    out[count] = '\0';
    return (int)count;
}