#ifndef KEYMAP_H
#define KEYMAP_H

#include "libc/types.h"
#include "keycodes.h"

u8 keymap_translate_ascii(keycode_t key, bool shift, bool caps_lock);

#endif
