#pragma once

#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* Keyboard driver                                                            */
/* -------------------------------------------------------------------------- */

void kbd_init(void);
void kbd_flush(void);

void kbd_push(int key);

int kbd_try_getchar(void);
int kbd_has_char(void);

/* Raw blocking read */
int kbd_read(void *buf, usize len);

/* -------------------------------------------------------------------------- */
/* Line editor                                                                */
/* -------------------------------------------------------------------------- */

/*
 * Read a line of keyboard input into buf.
 *
 * - Always NUL terminates.
 * - Echoes characters.
 * - Supports Backspace/Delete.
 * - Stores the newline.
 * - block != 0 waits until a full line is entered.
 * - block == 0 returns immediately with whatever is available.
 */
int kbd_read_line(char *buf, int count, int block);

/* -------------------------------------------------------------------------- */
/* History                                                                    */
/* -------------------------------------------------------------------------- */

#define KBD_HISTORY_DEPTH 16
#define KBD_LINE_MAX      128

typedef struct
{
    char entries[KBD_HISTORY_DEPTH][KBD_LINE_MAX];
    int count;
    int cursor; /* -1 while entering a new line */
} kbd_history_t;

void kbd_history_init(kbd_history_t *h);
void kbd_history_push(kbd_history_t *h, const char *line);

/* -------------------------------------------------------------------------- */
/* Completion                                                                 */
/* -------------------------------------------------------------------------- */

typedef const char *(*kbd_complete_fn)(const char *prefix, int attempt);

/*
Future:

const char *kbd_history_prev(kbd_history_t *h);
const char *kbd_history_next(kbd_history_t *h);

int kbd_read_line_edit(
    char *buf,
    int count,
    kbd_history_t *history,
    kbd_complete_fn complete);
*/