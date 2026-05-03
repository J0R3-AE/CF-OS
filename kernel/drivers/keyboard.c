/* drivers/keyboard.c */
#include "drivers/keyboard.h"
#include "drivers/kbd.h"
#include "arch/io.h"
#include "arch/pic.h"
#include <stdbool.h>
#include <stdint.h>

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64

static bool shift_down = false;
static bool ctrl_down = false;
static bool alt_down = false;
static bool caps_lock = false;
static bool extended = false;

static int translate_scancode(uint8_t code)
{
    bool upper = (shift_down ^ caps_lock);

    switch (code)
    {
    /* number row */
    case 0x02:
        return shift_down ? '!' : '1';
    case 0x03:
        return shift_down ? '@' : '2';
    case 0x04:
        return shift_down ? '#' : '3';
    case 0x05:
        return shift_down ? '$' : '4';
    case 0x06:
        return shift_down ? '%' : '5';
    case 0x07:
        return shift_down ? '^' : '6';
    case 0x08:
        return shift_down ? '&' : '7';
    case 0x09:
        return shift_down ? '*' : '8';
    case 0x0A:
        return shift_down ? '(' : '9';
    case 0x0B:
        return shift_down ? ')' : '0';
    case 0x0C:
        return shift_down ? '_' : '-';
    case 0x0D:
        return shift_down ? '+' : '=';
    case 0x0E:
        return KEY_BACKSPACE;

    /* top row */
    case 0x10:
        return upper ? 'Q' : 'q';
    case 0x11:
        return upper ? 'W' : 'w';
    case 0x12:
        return upper ? 'E' : 'e';
    case 0x13:
        return upper ? 'R' : 'r';
    case 0x14:
        return upper ? 'T' : 't';
    case 0x15:
        return upper ? 'Y' : 'y';
    case 0x16:
        return upper ? 'U' : 'u';
    case 0x17:
        return upper ? 'I' : 'i';
    case 0x18:
        return upper ? 'O' : 'o';
    case 0x19:
        return upper ? 'P' : 'p';
    case 0x1A:
        return shift_down ? '{' : '[';
    case 0x1B:
        return shift_down ? '}' : ']';

    /* home row */
    case 0x1E:
        return upper ? 'A' : 'a';
    case 0x1F:
        return upper ? 'S' : 's';
    case 0x20:
        return upper ? 'D' : 'd';
    case 0x21:
        return upper ? 'F' : 'f';
    case 0x22:
        return upper ? 'G' : 'g';
    case 0x23:
        return upper ? 'H' : 'h';
    case 0x24:
        return upper ? 'J' : 'j';
    case 0x25:
        return upper ? 'K' : 'k';
    case 0x26:
        return upper ? 'L' : 'l';
    case 0x27:
        return shift_down ? ':' : ';';
    case 0x28:
        return shift_down ? '"' : '\'';
    case 0x29:
        return shift_down ? '~' : '`';

    /* bottom row */
    case 0x2B:
        return shift_down ? '|' : '\\';
    case 0x2C:
        return upper ? 'Z' : 'z';
    case 0x2D:
        return upper ? 'X' : 'x';
    case 0x2E:
        return upper ? 'C' : 'c';
    case 0x2F:
        return upper ? 'V' : 'v';
    case 0x30:
        return upper ? 'B' : 'b';
    case 0x31:
        return upper ? 'N' : 'n';
    case 0x32:
        return upper ? 'M' : 'm';
    case 0x33:
        return shift_down ? '<' : ',';
    case 0x34:
        return shift_down ? '>' : '.';
    case 0x35:
        return shift_down ? '?' : '/';

    case 0x39:
        return ' ';

    /* special keys */
    case 0x1C:
        return KEY_ENTER;
    case 0x01:
        return KEY_ESC;
    case 0x0F:
        return KEY_TAB;

    default:
        return KEY_NONE;
    }
}

static void handle_scancode(uint8_t scancode)
{
    /* extended prefix */
    if (scancode == 0xE0)
    {
        extended = true;
        return;
    }

    /* extended keys */
    if (extended)
    {
        extended = false;

        switch (scancode)
        {
        case 0x48:
            kbd_push(KEY_UP);
            return;
        case 0x50:
            kbd_push(KEY_DOWN);
            return;
        case 0x4B:
            kbd_push(KEY_LEFT);
            return;
        case 0x4D:
            kbd_push(KEY_RIGHT);
            return;
        default:
            return;
        }
    }

    bool released = (scancode & 0x80u) != 0;
    uint8_t code = (uint8_t)(scancode & 0x7Fu);

    /* modifier keys */
    if (code == 0x2A || code == 0x36) /* shift */
    {
        shift_down = !released;
        return;
    }
    if (code == 0x1D) /* ctrl */
    {
        ctrl_down = !released;
        return;
    }
    if (code == 0x38) /* alt */
    {
        alt_down = !released;
        return;
    }
    if (code == 0x3A && !released) /* caps lock */
    {
        caps_lock = !caps_lock;
        return;
    }

    /* ignore key releases */
    if (released)
        return;

    int key = translate_scancode(code);
    if (key != KEY_NONE)
        kbd_push(key);
}

void keyboard_init(void)
{
    shift_down = false;
    ctrl_down = false;
    alt_down = false;
    caps_lock = false;
    extended = false;
}

void keyboard_irq_handler(struct registers *r)
{
    (void)*r; // unused

    while (io_Read8(KBD_STATUS_PORT) & 0x01u)
    {
        uint8_t scancode = io_Read8(KBD_DATA_PORT);
        handle_scancode(scancode);
    }

    pic_send_eoi(1);
}