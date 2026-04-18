#pragma once
#include "libk/types.h"

/* printable */
#define KEY_NONE        0
#define KEY_BACKSPACE   8
#define KEY_TAB         9
#define KEY_ENTER       10
#define KEY_ESC         27

/* arrows (extended keys) */
#define KEY_UP          1001
#define KEY_DOWN        1002
#define KEY_LEFT        1003
#define KEY_RIGHT       1004

void kbd_init(void);
void kbd_flush(void);

void kbd_push(int key);

int kbd_try_getchar(void);
int kbd_has_char(void);

int kbd_read(void *buf, usize len);