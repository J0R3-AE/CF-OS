/*
 * kbd.h - public interface of the keyboard input subsystem
 */

#ifndef KBD_H
#define KBD_H

#include <stddef.h>
#include "scancode.h"

/* ── History configuration ────────────────────────────────────────────────── */

#define KBD_HISTORY_MAX   64
#define KBD_HISTORY_LEN   256   /* max chars per history entry */

/* ── Subsystem API ────────────────────────────────────────────────────────── */

void kbd_init(void);
void kbd_flush(void);

/* Called from scancode_irq_handler – may be called in interrupt context. */
void kbd_push(const struct key_event *ev);

/* Non-blocking read: returns 0 if nothing available. */
int  kbd_try_getchar(char *ch_out);

/* Returns 1 if at least one character is waiting. */
int  kbd_has_char(void);

/* Blocking reads – yield scheduler while waiting. */
char        kbd_read(void);
size_t      kbd_read_line(char *buf, size_t size);

/* History */
void        kbd_history_add(const char *line);
const char *kbd_history_get(int offset);   /* 0 = most recent */
int         kbd_history_count(void);
void        kbd_history_clear(void);

#endif /* KBD_H */
