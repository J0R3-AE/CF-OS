/*
 * kbd.c - keyboard input subsystem
 *
 * Sits above scancode.c.  Maintains a ring buffer of pending characters,
 * performs cooked line editing (backspace, delete, enter, escape), stores
 * command history, and provides both blocking and non-blocking read helpers.
 *
 * The subsystem is completely independent of the PS/2 hardware; it only
 * consumes struct key_event values produced by scancode.c.
 *
 * Blocking calls (kbd_read, kbd_read_line) assume the platform exposes a
 * scheduler_yield() function that suspends the current task until the next
 * interrupt fires.  Replace that call if your kernel uses a different
 * mechanism.
 */

#include "drivers/kbd.h"
#include "libk/string.h"
#include "drivers/tty.h"

/* ── Platform stub – replace with your kernel's scheduler yield ───────────── */

extern void ksched_yield(void);

/* ── TTY output stub – replace with your terminal write function ──────────── */


/* ── Ring-buffer configuration ────────────────────────────────────────────── */

#define KBD_RING_SIZE 256   /* must be a power of two */
#define KBD_RING_MASK (KBD_RING_SIZE - 1)

/* ── Internal state ───────────────────────────────────────────────────────── */

/* Character ring buffer shared between IRQ context (writer) and task (reader) */
static volatile char    ring_buf[KBD_RING_SIZE];
static volatile size_t  ring_head = 0;  /* next write position */
static volatile size_t  ring_tail = 0;  /* next read  position */

/* Command history */
static char   history[KBD_HISTORY_MAX][KBD_HISTORY_LEN];
static int    history_count = 0;
static int    history_head  = 0;   /* index of the most recently added entry */

/* ── Ring-buffer helpers ──────────────────────────────────────────────────── */

static inline int ring_full(void)
{
    return ((ring_head + 1) & KBD_RING_MASK) == ring_tail;
}

static inline int ring_empty(void)
{
    return ring_head == ring_tail;
}

static inline void ring_push(char c)
{
    if (!ring_full()) {
        ring_buf[ring_head] = c;
        ring_head = (ring_head + 1) & KBD_RING_MASK;
    }
    /* If full we silently drop – never block in IRQ context. */
}

static inline char ring_pop(void)
{
    char c = ring_buf[ring_tail];
    ring_tail = (ring_tail + 1) & KBD_RING_MASK;
    return c;
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void kbd_init(void)
{
    ring_head    = 0;
    ring_tail    = 0;
    history_count = 0;
    history_head  = 0;
    memset(history, 0, sizeof(history));
}

void kbd_flush(void)
{
    ring_head = ring_tail = 0;
}

/*
 * kbd_push() - called from scancode_irq_handler() (interrupt context).
 *
 * Only key-press events with a printable (or control) ASCII value are
 * enqueued; releases and non-ASCII keys are silently ignored here.
 */
void kbd_push(const struct key_event *ev)
{
    if (!ev->pressed)
        return;

    if (ev->ascii != 0)
    {
        ring_push(ev->ascii);
        return;
    }

    /* push special keys as encoded values */
    ring_push((char)(0x80 | ev->keycode));
}


int kbd_has_char(void)
{
    return !ring_empty();
}

int kbd_try_getchar(char *ch_out)
{
    if (ring_empty())
        return 0;
    *ch_out = ring_pop();
    return 1;
}

/*
 * kbd_read() - blocking single-character read.
 *
 * Yields the CPU until a character arrives in the ring buffer.
 */
char kbd_read(void)
{
    while (ring_empty())
        //ksched_yield();
    return ring_pop();
}

/* ── Cooked line editing ──────────────────────────────────────────────────── */

/*
 * kbd_read_line() - blocking cooked-mode line read.
 *
 * Reads characters into buf (up to size-1 bytes) until the user presses
 * Enter.  Handles:
 *   \b / DEL  back up and erase one character
 *   \n / \r   end of line
 *   ESC       discard the current line and start fresh
 *
 * The returned string is always NUL-terminated.  The trailing newline is
 * NOT included in buf.  Returns the number of characters written (excluding
 * NUL).
 *
 * The completed line is automatically added to the history.
 */
size_t kbd_read_line(char *buf, size_t size)
{
    if (size == 0)
        return 0;

    size_t len = 0;

    for (;;) {
        char c = kbd_read();

        switch (c) {
        case '\n':
        case '\r':
            TTY_putc('\n');
            buf[len] = '\0';
            if (len > 0)
                kbd_history_add(buf);
            return len;

        case '\b':      /* Backspace */
        case 0x7F:      /* DEL – also treated as backspace in cooked mode */
            if (len > 0) {
                len--;
                /* Erase the character on the terminal: back, space, back. */
                TTY_putc('\b');
                TTY_putc(' ');
                TTY_putc('\b');
            }
            break;

        case 0x1B:      /* ESC – discard line */
            /* Visually clear the line. */
            while (len > 0) {
                TTY_putc('\b');
                TTY_putc(' ');
                TTY_putc('\b');
                len--;
            }
            break;

        default:
            if (c < 0x20)
                break;  /* ignore other control characters */
            if (len < size - 1) {
                buf[len++] = c;
                TTY_putc(c);     /* echo */
            }
            /* If the buffer is full we silently drop additional input. */
            break;
        }
    }
}

/* ── Command history ──────────────────────────────────────────────────────── */

/*
 * History is stored as a circular array of fixed-size strings.
 * history_head always points to the slot that will receive the NEXT entry.
 * history_count tracks how many valid entries exist (capped at
 * KBD_HISTORY_MAX).
 */

void kbd_history_add(const char *line)
{
    if (!line || *line == '\0')
        return;

    /* Don't add a duplicate of the most recent entry. */
    if (history_count > 0) {
        int prev = (history_head - 1 + KBD_HISTORY_MAX) % KBD_HISTORY_MAX;
        if (strncmp(history[prev], line, KBD_HISTORY_LEN - 1) == 0)
            return;
    }

    strncpy(history[history_head], line, KBD_HISTORY_LEN - 1);
    history[history_head][KBD_HISTORY_LEN - 1] = '\0';

    history_head = (history_head + 1) % KBD_HISTORY_MAX;
    if (history_count < KBD_HISTORY_MAX)
        history_count++;
}

/*
 * kbd_history_get() - retrieve a past command.
 *
 * offset 0 = most recent entry, 1 = one before that, etc.
 * Returns NULL if offset is out of range.
 */
const char *kbd_history_get(int offset)
{
    if (offset < 0 || offset >= history_count)
        return NULL;

    int idx = (history_head - 1 - offset + KBD_HISTORY_MAX * 2) % KBD_HISTORY_MAX;
    return history[idx];
}

int kbd_history_count(void)
{
    return history_count;
}

void kbd_history_clear(void)
{
    history_count = 0;
    history_head  = 0;
    memset(history, 0, sizeof(history));
}
