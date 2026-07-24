#include <stdint.h>
#include "kernel/drivers/keycodes.h"
#include "kernel/drivers/keyevent.h"
#include "kernel/drivers/keymap.h"
#include "kernel/drivers/line.h"

#include "kernel/drivers/ps2/ps2.h"   // we'll define this below

// Simple set 1 scancode → keycode map (partial)
static keycode_t scancode_set1[256] = {
    [0x1C] = KEY_ENTER,
    [0x39] = KEY_SPACE,
    [0x0E] = KEY_BACKSPACE,
    [0x1E] = KEY_A,
    [0x30] = KEY_B,
    [0x2E] = KEY_C,
    // ... fill out as needed
};

static int shift_down = 0;

static keycode_t translate_scancode(uint8_t sc)
{
    return scancode_set1[sc];
}

static uint8_t translate_ascii(keycode_t key)
{
    uint8_t base = keymap_get_ascii(key);
    if (!base)
        return 0;

    if (shift_down && base >= 'a' && base <= 'z')
        return base - 'a' + 'A';

    return base;
}

void keyboard_handle_scancode(uint8_t sc)
{
    key_event_t ev = {0};

    // release bit
    int release = sc & 0x80;
    uint8_t code = sc & 0x7F;

    keycode_t key = translate_scancode(code);
    if (key == KEY_NONE)
        return;

    if (key == KEY_LSHIFT || key == KEY_RSHIFT) {
        shift_down = !release;
        return;
    }

    ev.keycode = key;
    ev.type    = release ? KEY_EVENT_RELEASE : KEY_EVENT_PRESS;
    ev.ascii   = translate_ascii(key);

    line_handle_keyevent(&ev);
}

void keyboard_init(void)
{
    // hook into PS/2 driver
    ps2_register_keyboard_callback(keyboard_handle_scancode);
}
