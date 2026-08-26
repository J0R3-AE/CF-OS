#include <stdint.h>

#include "kernel/drivers/keycodes.h"
#include "kernel/drivers/keyevent.h"
#include "kernel/drivers/keymap.h"
#include "kernel/drivers/line.h"
#include "kernel/drivers/ps2/ps2.h"

/*
 * PS/2 Set 1 keyboard driver.
 *
 * Responsibilities:
 *   - Decode Set 1 scancodes.
 *   - Handle E0 extended scancodes.
 *   - Track modifier state.
 *   - Track Caps Lock.
 *   - Convert key presses to ASCII through the keymap.
 *   - Produce key_event_t events for the line layer.
 *
 * This driver does NOT:
 *   - Parse shell commands.
 *   - Handle signals.
 *   - Manage TTY state.
 */

static int left_shift_down  = 0;
static int right_shift_down = 0;
static int ctrl_down        = 0;
static int alt_down         = 0;
static int caps_lock        = 0;

static int extended_scancode = 0;


/*
 * Set 1 base scancode -> keycode.
 *
 * Only make codes are stored here. Release is determined from
 * the high bit of the incoming scancode.
 */
static const keycode_t scancode_set1[128] =
{
    [0x01] = KEY_ESC,

    [0x02] = KEY_1,
    [0x03] = KEY_2,
    [0x04] = KEY_3,
    [0x05] = KEY_4,
    [0x06] = KEY_5,
    [0x07] = KEY_6,
    [0x08] = KEY_7,
    [0x09] = KEY_8,
    [0x0A] = KEY_9,
    [0x0B] = KEY_0,

    [0x0E] = KEY_BACKSPACE,
    [0x0F] = KEY_TAB,

    [0x10] = KEY_Q,
    [0x11] = KEY_W,
    [0x12] = KEY_E,
    [0x13] = KEY_R,
    [0x14] = KEY_T,
    [0x15] = KEY_Y,
    [0x16] = KEY_U,
    [0x17] = KEY_I,
    [0x18] = KEY_O,
    [0x19] = KEY_P,

    [0x1C] = KEY_ENTER,

    [0x1E] = KEY_A,
    [0x1F] = KEY_S,
    [0x20] = KEY_D,
    [0x21] = KEY_F,
    [0x22] = KEY_G,
    [0x23] = KEY_H,
    [0x24] = KEY_J,
    [0x25] = KEY_K,
    [0x26] = KEY_L,

    [0x2A] = KEY_LSHIFT,

    [0x2C] = KEY_Z,
    [0x2D] = KEY_X,
    [0x2E] = KEY_C,
    [0x2F] = KEY_V,
    [0x30] = KEY_B,
    [0x31] = KEY_N,
    [0x32] = KEY_M,

    [0x36] = KEY_RSHIFT,
    [0x38] = KEY_LALT,

    [0x39] = KEY_SPACE,

    [0x3A] = KEY_CAPSLOCK,

    [0x3B] = KEY_F1,
    [0x3C] = KEY_F2,
    [0x3D] = KEY_F3,
    [0x3E] = KEY_F4,
    [0x3F] = KEY_F5,
    [0x40] = KEY_F6,
    [0x41] = KEY_F7,
    [0x42] = KEY_F8,
    [0x43] = KEY_F9,
    [0x44] = KEY_F10,
    [0x57] = KEY_F11,
    [0x58] = KEY_F12,
};


/*
 * Translate a normal Set 1 scancode.
 */
static keycode_t translate_scancode(uint8_t code)
{
    if (code >= 128)
        return KEY_NONE;

    return scancode_set1[code];
}


/*
 * Translate E0-prefixed scancodes.
 */
static keycode_t translate_extended_scancode(uint8_t code)
{
    switch (code)
    {
        case 0x48:
            return KEY_UP;

        case 0x50:
            return KEY_DOWN;

        case 0x4B:
            return KEY_LEFT;

        case 0x4D:
            return KEY_RIGHT;

        /*
         * Right Ctrl uses E0 1D.
         * We only have KEY_LCTRL in your current keycodes,
         * so represent it as Ctrl for now.
         */
        case 0x1D:
            return KEY_LCTRL;

        /*
         * Right Alt / AltGr uses E0 38.
         * Again, your current keycodes only define LALT.
         */
        case 0x38:
            return KEY_LALT;

        default:
            return KEY_NONE;
    }
}


/*
 * Check whether Shift is currently held.
 */
static int shift_is_down(void)
{
    return left_shift_down || right_shift_down;
}


/*
 * Update modifier state.
 */
static void update_modifier_state(
    keycode_t key,
    int release)
{
    switch (key)
    {
        case KEY_LSHIFT:
            left_shift_down = !release;
            break;

        case KEY_RSHIFT:
            right_shift_down = !release;
            break;

        case KEY_LCTRL:
            ctrl_down = !release;
            break;

        case KEY_LALT:
            alt_down = !release;
            break;

        case KEY_CAPSLOCK:
            /*
             * Caps Lock changes state on press only.
             */
            if (!release)
                caps_lock = !caps_lock;
            break;

        default:
            break;
    }
}


/*
 * Translate a keycode into ASCII while respecting
 * Shift and Caps Lock.
 */
static uint8_t translate_ascii(keycode_t key)
{
    uint8_t base;

    base = keymap_get_ascii(key);

    if (!base)
        return 0;

    /*
     * Letters:
     *
     * Shift XOR Caps Lock determines uppercase.
     */
    if (base >= 'a' && base <= 'z')
    {
        int uppercase =
            shift_is_down() ^ caps_lock;

        if (uppercase)
            return (uint8_t)(base - 'a' + 'A');

        return base;
    }

    /*
     * Number-row / punctuation Shift mappings.
     */
    if (shift_is_down())
    {
        switch (key)
        {
            case KEY_1: return '!';
            case KEY_2: return '@';
            case KEY_3: return '#';
            case KEY_4: return '$';
            case KEY_5: return '%';
            case KEY_6: return '^';
            case KEY_7: return '&';
            case KEY_8: return '*';
            case KEY_9: return '(';
            case KEY_0: return ')';

            default:
                break;
        }
    }

    /*
     * Ctrl+letter.
     *
     * This is especially important for the TTY:
     *
     *   Ctrl+C = 0x03
     *   Ctrl+D = 0x04
     *   Ctrl+Z = 0x1A
     *
     * The line/TTY layer can turn these into signals later.
     */
    if (ctrl_down && base >= 'a' && base <= 'z')
    {
        return (uint8_t)(base - 'a' + 1);
    }

    return base;
}


/*
 * Main PS/2 keyboard callback.
 */
void keyboard_handle_scancode(uint8_t sc)
{
    uint8_t raw = sc;
    uint8_t code;
    int release;
    keycode_t key;
    key_event_t ev;

    if (raw == 0xE0)
    {
        extended_scancode = 1;
        return;
    }

    if (raw == 0xE1)
    {
        extended_scancode = 0;
        return;
    }

    release = (raw & 0x80) != 0;
    code = raw & 0x7F;

    if (extended_scancode)
    {
        key = translate_extended_scancode(code);
        extended_scancode = 0;
    }
    else
    {
        key = translate_scancode(code);
    }

    if (key == KEY_NONE)
        return;

    update_modifier_state(key, release);

    ev.type = release
        ? KEY_EVENT_RELEASE
        : KEY_EVENT_PRESS;

    ev.keycode = key;
    ev.ascii = release ? 0 : translate_ascii(key);

    line_handle_keyevent(&ev);
}

/*
 * Initialize keyboard state and register the PS/2 callback.
 */
void keyboard_init(void)
{
    left_shift_down  = 0;
    right_shift_down = 0;
    ctrl_down        = 0;
    alt_down         = 0;
    caps_lock        = 0;
    extended_scancode = 0;

    ps2_register_keyboard_callback(
        keyboard_handle_scancode);
}