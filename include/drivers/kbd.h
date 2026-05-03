#pragma once
#include "libk/types.h"

void kbd_init(void);
void kbd_flush(void);

void kbd_push(int key);

int kbd_try_getchar(void);
int kbd_has_char(void);

int kbd_read(void *buf, usize len);