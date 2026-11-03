#include "kernel/drivers/keyboard.h"
#include "kernel/drivers/keycodes.h"
#include "kernel/drivers/keyevent.h"
#include "kernel/drivers/keymap.h"
#include "kernel/drivers/line.h"
#include "kernel/drivers/ps2/ps2.h"

static bool left_shift;
static bool right_shift;
static bool caps_lock;
static bool extended_scancode;

static keycode_t translate_scancode(u8 code, bool extended)
{
    if (extended)
    {
        switch (code)
        {
        case 0x1C: return KEY_ENTER;
        case 0x1D: return KEY_RCTRL;
        case 0x38: return KEY_RALT;
        case 0x47: return KEY_HOME;
        case 0x48: return KEY_UP;
        case 0x49: return KEY_PAGE_UP;
        case 0x4B: return KEY_LEFT;
        case 0x4D: return KEY_RIGHT;
        case 0x4F: return KEY_END;
        case 0x50: return KEY_DOWN;
        case 0x51: return KEY_PAGE_DOWN;
        case 0x53: return KEY_DELETE;
        default: return KEY_NONE;
        }
    }

    switch (code)
    {
    case 0x01: return KEY_ESC;
    case 0x02: return KEY_1;
    case 0x03: return KEY_2;
    case 0x04: return KEY_3;
    case 0x05: return KEY_4;
    case 0x06: return KEY_5;
    case 0x07: return KEY_6;
    case 0x08: return KEY_7;
    case 0x09: return KEY_8;
    case 0x0A: return KEY_9;
    case 0x0B: return KEY_0;
    case 0x0C: return KEY_MINUS;
    case 0x0D: return KEY_EQUALS;
    case 0x0E: return KEY_BACKSPACE;
    case 0x0F: return KEY_TAB;
    case 0x10: return KEY_Q;
    case 0x11: return KEY_W;
    case 0x12: return KEY_E;
    case 0x13: return KEY_R;
    case 0x14: return KEY_T;
    case 0x15: return KEY_Y;
    case 0x16: return KEY_U;
    case 0x17: return KEY_I;
    case 0x18: return KEY_O;
    case 0x19: return KEY_P;
    case 0x1A: return KEY_LEFT_BRACKET;
    case 0x1B: return KEY_RIGHT_BRACKET;
    case 0x1C: return KEY_ENTER;
    case 0x1D: return KEY_LCTRL;
    case 0x1E: return KEY_A;
    case 0x1F: return KEY_S;
    case 0x20: return KEY_D;
    case 0x21: return KEY_F;
    case 0x22: return KEY_G;
    case 0x23: return KEY_H;
    case 0x24: return KEY_J;
    case 0x25: return KEY_K;
    case 0x26: return KEY_L;
    case 0x27: return KEY_SEMICOLON;
    case 0x28: return KEY_APOSTROPHE;
    case 0x29: return KEY_GRAVE;
    case 0x2A: return KEY_LSHIFT;
    case 0x2B: return KEY_BACKSLASH;
    case 0x2C: return KEY_Z;
    case 0x2D: return KEY_X;
    case 0x2E: return KEY_C;
    case 0x2F: return KEY_V;
    case 0x30: return KEY_B;
    case 0x31: return KEY_N;
    case 0x32: return KEY_M;
    case 0x33: return KEY_COMMA;
    case 0x34: return KEY_PERIOD;
    case 0x35: return KEY_SLASH;
    case 0x36: return KEY_RSHIFT;
    case 0x38: return KEY_LALT;
    case 0x39: return KEY_SPACE;
    case 0x3A: return KEY_CAPSLOCK;
    default: return KEY_NONE;
    }
}

void keyboard_handle_scancode(u8 scancode)
{
    key_event_t event;
    keycode_t key;
    bool released;

    if (scancode == 0xE0)
    {
        extended_scancode = true;
        return;
    }
    if (scancode == 0xE1)
    {
        extended_scancode = false;
        return;
    }

    released = (scancode & 0x80) != 0;
    key = translate_scancode(scancode & 0x7F, extended_scancode);
    extended_scancode = false;

    if (key == KEY_LSHIFT)
    {
        left_shift = !released;
        return;
    }
    if (key == KEY_RSHIFT)
    {
        right_shift = !released;
        return;
    }
    if (key == KEY_CAPSLOCK)
    {
        if (!released)
            caps_lock = !caps_lock;
        return;
    }
    if (key == KEY_NONE || key == KEY_ESC ||
        key == KEY_LCTRL || key == KEY_RCTRL ||
        key == KEY_LALT || key == KEY_RALT)
        return;

    event.keycode = key;
    event.type = released ? KEY_EVENT_RELEASE : KEY_EVENT_PRESS;
    event.ascii = released ? 0 :
        keymap_translate_ascii(key, left_shift || right_shift, caps_lock);
    line_handle_keyevent(&event);
}

void keyboard_init(void)
{
    left_shift = false;
    right_shift = false;
    caps_lock = false;
    extended_scancode = false;
    line_reset();
    ps2_register_keyboard_callback(keyboard_handle_scancode);
}
