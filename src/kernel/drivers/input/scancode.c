/*
 * scancode.c - PS/2 keyboard hardware driver
 *
 * Translates raw PS/2 scan codes (set 1) into generic key events and
 * forwards them to the keyboard subsystem (kbd.c).  This file never
 * blocks, never echoes, and never touches the TTY/shell layer.
 */

#include "drivers/scancode.h"
#include "drivers/kbd.h"

/* ── I/O port helpers ─────────────────────────────────────────────────────── */

static inline uint8_t inb(uint16_t port)
{
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* ── PS/2 port constants ──────────────────────────────────────────────────── */

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64

#define PS2_STATUS_OBF  0x01    /* output buffer full – safe to read */

/* ── PIC constants ────────────────────────────────────────────────────────── */

#define PIC1_CMD  0x20
#define PIC_EOI   0x20

/* ── Scan-code prefixes ───────────────────────────────────────────────────── */

#define SC_PREFIX_E0   0xE0
#define SC_PREFIX_E1   0xE1
#define SC_RELEASE     0x80    /* bit 7 set → key release */

/* ── Modifier flags ───────────────────────────────────────────────────────── */

#define MOD_SHIFT      (1 << 0)
#define MOD_CTRL       (1 << 1)
#define MOD_ALT        (1 << 2)
#define MOD_CAPS_LOCK  (1 << 3)

/* ── Driver state ─────────────────────────────────────────────────────────── */

static uint8_t  sc_modifiers   = 0;
static uint8_t  sc_prefix      = 0;    /* 0, 0xE0, or 0xE1          */
static uint8_t  sc_e1_buf[2]   = {0};  /* partial E1 sequence bytes  */
static uint8_t  sc_e1_count    = 0;

/* ── Scan-code-set-1 → keycode tables ────────────────────────────────────── */

/* Base (no prefix) */
static const uint8_t sc_to_key[128] = {
    [0x01] = KEY_ESCAPE,
    [0x02] = KEY_1,       [0x03] = KEY_2,       [0x04] = KEY_3,
    [0x05] = KEY_4,       [0x06] = KEY_5,       [0x07] = KEY_6,
    [0x08] = KEY_7,       [0x09] = KEY_8,       [0x0A] = KEY_9,
    [0x0B] = KEY_0,       [0x0C] = KEY_MINUS,   [0x0D] = KEY_EQUALS,
    [0x0E] = KEY_BACKSPACE,
    [0x0F] = KEY_TAB,
    [0x10] = KEY_Q,       [0x11] = KEY_W,       [0x12] = KEY_E,
    [0x13] = KEY_R,       [0x14] = KEY_T,       [0x15] = KEY_Y,
    [0x16] = KEY_U,       [0x17] = KEY_I,       [0x18] = KEY_O,
    [0x19] = KEY_P,       [0x1A] = KEY_LBRACKET,[0x1B] = KEY_RBRACKET,
    [0x1C] = KEY_ENTER,
    [0x1D] = KEY_LCTRL,
    [0x1E] = KEY_A,       [0x1F] = KEY_S,       [0x20] = KEY_D,
    [0x21] = KEY_F,       [0x22] = KEY_G,       [0x23] = KEY_H,
    [0x24] = KEY_J,       [0x25] = KEY_K,       [0x26] = KEY_L,
    [0x27] = KEY_SEMICOLON,[0x28] = KEY_APOSTROPHE,
    [0x29] = KEY_GRAVE,
    [0x2A] = KEY_LSHIFT,
    [0x2B] = KEY_BACKSLASH,
    [0x2C] = KEY_Z,       [0x2D] = KEY_X,       [0x2E] = KEY_C,
    [0x2F] = KEY_V,       [0x30] = KEY_B,       [0x31] = KEY_N,
    [0x32] = KEY_M,       [0x33] = KEY_COMMA,   [0x34] = KEY_DOT,
    [0x35] = KEY_SLASH,
    [0x36] = KEY_RSHIFT,
    [0x37] = KEY_KP_STAR,
    [0x38] = KEY_LALT,
    [0x39] = KEY_SPACE,
    [0x3A] = KEY_CAPS_LOCK,
    [0x3B] = KEY_F1,  [0x3C] = KEY_F2,  [0x3D] = KEY_F3,  [0x3E] = KEY_F4,
    [0x3F] = KEY_F5,  [0x40] = KEY_F6,  [0x41] = KEY_F7,  [0x42] = KEY_F8,
    [0x43] = KEY_F9,  [0x44] = KEY_F10,
    [0x45] = KEY_NUM_LOCK,
    [0x46] = KEY_SCROLL_LOCK,
    [0x47] = KEY_KP_7, [0x48] = KEY_KP_8, [0x49] = KEY_KP_9,
    [0x4A] = KEY_KP_MINUS,
    [0x4B] = KEY_KP_4, [0x4C] = KEY_KP_5, [0x4D] = KEY_KP_6,
    [0x4E] = KEY_KP_PLUS,
    [0x4F] = KEY_KP_1, [0x50] = KEY_KP_2, [0x51] = KEY_KP_3,
    [0x52] = KEY_KP_0, [0x53] = KEY_KP_DOT,
    [0x57] = KEY_F11, [0x58] = KEY_F12,
};

/* Extended (0xE0 prefix) */
static const uint8_t sc_e0_to_key[128] = {
    [0x1C] = KEY_KP_ENTER,
    [0x1D] = KEY_RCTRL,
    [0x35] = KEY_KP_SLASH,
    [0x38] = KEY_RALT,
    [0x47] = KEY_HOME,
    [0x48] = KEY_UP,
    [0x49] = KEY_PAGE_UP,
    [0x4B] = KEY_LEFT,
    [0x4D] = KEY_RIGHT,
    [0x4F] = KEY_END,
    [0x50] = KEY_DOWN,
    [0x51] = KEY_PAGE_DOWN,
    [0x52] = KEY_INSERT,
    [0x53] = KEY_DELETE,
    [0x5B] = KEY_LSUPER,
    [0x5C] = KEY_RSUPER,
    [0x5D] = KEY_MENU,
};

/* ── Unshifted / shifted ASCII maps ──────────────────────────────────────── */

static const char key_to_ascii_lower[KEY_COUNT] = {
    [KEY_SPACE]       = ' ',
    [KEY_1]           = '1', [KEY_2]     = '2', [KEY_3]    = '3',
    [KEY_4]           = '4', [KEY_5]     = '5', [KEY_6]    = '6',
    [KEY_7]           = '7', [KEY_8]     = '8', [KEY_9]    = '9',
    [KEY_0]           = '0',
    [KEY_MINUS]       = '-', [KEY_EQUALS]     = '=',
    [KEY_LBRACKET]    = '[', [KEY_RBRACKET]   = ']',
    [KEY_SEMICOLON]   = ';', [KEY_APOSTROPHE] = '\'',
    [KEY_GRAVE]       = '`', [KEY_BACKSLASH]  = '\\',
    [KEY_COMMA]       = ',', [KEY_DOT]        = '.', [KEY_SLASH] = '/',
    [KEY_TAB]         = '\t',
    [KEY_ENTER]       = '\n', [KEY_KP_ENTER]  = '\n',
    [KEY_BACKSPACE]   = '\b',
    [KEY_ESCAPE]      = 0x1B,
    [KEY_A] = 'a', [KEY_B] = 'b', [KEY_C] = 'c', [KEY_D] = 'd',
    [KEY_E] = 'e', [KEY_F] = 'f', [KEY_G] = 'g', [KEY_H] = 'h',
    [KEY_I] = 'i', [KEY_J] = 'j', [KEY_K] = 'k', [KEY_L] = 'l',
    [KEY_M] = 'm', [KEY_N] = 'n', [KEY_O] = 'o', [KEY_P] = 'p',
    [KEY_Q] = 'q', [KEY_R] = 'r', [KEY_S] = 's', [KEY_T] = 't',
    [KEY_U] = 'u', [KEY_V] = 'v', [KEY_W] = 'w', [KEY_X] = 'x',
    [KEY_Y] = 'y', [KEY_Z] = 'z',
    [KEY_KP_0] = '0', [KEY_KP_1] = '1', [KEY_KP_2] = '2',
    [KEY_KP_3] = '3', [KEY_KP_4] = '4', [KEY_KP_5] = '5',
    [KEY_KP_6] = '6', [KEY_KP_7] = '7', [KEY_KP_8] = '8',
    [KEY_KP_9] = '9', [KEY_KP_DOT] = '.', [KEY_KP_STAR] = '*',
    [KEY_KP_SLASH] = '/', [KEY_KP_PLUS] = '+', [KEY_KP_MINUS] = '-',
};

static const char key_to_ascii_upper[KEY_COUNT] = {
    [KEY_SPACE]       = ' ',
    [KEY_1]           = '!', [KEY_2]     = '@', [KEY_3]    = '#',
    [KEY_4]           = '$', [KEY_5]     = '%', [KEY_6]    = '^',
    [KEY_7]           = '&', [KEY_8]     = '*', [KEY_9]    = '(',
    [KEY_0]           = ')',
    [KEY_MINUS]       = '_', [KEY_EQUALS]     = '+',
    [KEY_LBRACKET]    = '{', [KEY_RBRACKET]   = '}',
    [KEY_SEMICOLON]   = ':', [KEY_APOSTROPHE] = '"',
    [KEY_GRAVE]       = '~', [KEY_BACKSLASH]  = '|',
    [KEY_COMMA]       = '<', [KEY_DOT]        = '>', [KEY_SLASH] = '?',
    [KEY_TAB]         = '\t',
    [KEY_ENTER]       = '\n', [KEY_KP_ENTER]  = '\n',
    [KEY_BACKSPACE]   = '\b',
    [KEY_ESCAPE]      = 0x1B,
    [KEY_A] = 'A', [KEY_B] = 'B', [KEY_C] = 'C', [KEY_D] = 'D',
    [KEY_E] = 'E', [KEY_F] = 'F', [KEY_G] = 'G', [KEY_H] = 'H',
    [KEY_I] = 'I', [KEY_J] = 'J', [KEY_K] = 'K', [KEY_L] = 'L',
    [KEY_M] = 'M', [KEY_N] = 'N', [KEY_O] = 'O', [KEY_P] = 'P',
    [KEY_Q] = 'Q', [KEY_R] = 'R', [KEY_S] = 'S', [KEY_T] = 'T',
    [KEY_U] = 'U', [KEY_V] = 'V', [KEY_W] = 'W', [KEY_X] = 'X',
    [KEY_Y] = 'Y', [KEY_Z] = 'Z',
    [KEY_KP_0] = '0', [KEY_KP_1] = '1', [KEY_KP_2] = '2',
    [KEY_KP_3] = '3', [KEY_KP_4] = '4', [KEY_KP_5] = '5',
    [KEY_KP_6] = '6', [KEY_KP_7] = '7', [KEY_KP_8] = '8',
    [KEY_KP_9] = '9', [KEY_KP_DOT] = '.', [KEY_KP_STAR] = '*',
    [KEY_KP_SLASH] = '/', [KEY_KP_PLUS] = '+', [KEY_KP_MINUS] = '-',
};

/* ── Modifier tracking ────────────────────────────────────────────────────── */

static void update_modifiers(uint8_t keycode, int pressed)
{
    uint8_t bit = 0;
    switch (keycode) {
    case KEY_LSHIFT: case KEY_RSHIFT: bit = MOD_SHIFT;     break;
    case KEY_LCTRL:  case KEY_RCTRL:  bit = MOD_CTRL;      break;
    case KEY_LALT:   case KEY_RALT:   bit = MOD_ALT;        break;
    case KEY_CAPS_LOCK:
        if (pressed)
            sc_modifiers ^= MOD_CAPS_LOCK;
        return;
    default:
        return;
    }
    if (pressed)
        sc_modifiers |= bit;
    else
        sc_modifiers &= ~bit;
}

/* ── ASCII derivation ─────────────────────────────────────────────────────── */

/*
 * Caps Lock flips case for letters only; Shift flips everything else and
 * also inverts the Caps Lock state for letters.
 */
static char derive_ascii(uint8_t keycode)
{
    int shift    = (sc_modifiers & MOD_SHIFT)    != 0;
    int caps     = (sc_modifiers & MOD_CAPS_LOCK) != 0;
    int is_alpha = (keycode >= KEY_A && keycode <= KEY_Z);

    int upper = shift ^ (is_alpha & caps);
    return upper ? key_to_ascii_upper[keycode]
                 : key_to_ascii_lower[keycode];
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void scancode_init(void)
{
    sc_modifiers = 0;
    sc_prefix    = 0;
    sc_e1_count  = 0;
    /* IRQ1 must be unmasked in the PIC by the caller's IRQ setup code. */
}

/*
 * scancode_translate() - exposed for unit testing; ordinarily called only
 * by scancode_irq_handler().
 *
 * Feed one raw byte from the PS/2 data port.  Returns 1 if a complete event
 * was produced and written to *ev_out, 0 otherwise.
 */
int scancode_translate(uint8_t raw, struct key_event *ev_out)
{
    /* ── Handle multi-byte prefixes ──────────────────────────────────── */
    if (raw == SC_PREFIX_E0) {
        sc_prefix = SC_PREFIX_E0;
        return 0;
    }
    if (raw == SC_PREFIX_E1) {
        sc_prefix   = SC_PREFIX_E1;
        sc_e1_count = 0;
        return 0;
    }

    /* Accumulate E1 sequence (Pause key: E1 1D 45 / E1 9D C5) */
    if (sc_prefix == SC_PREFIX_E1) {
        sc_e1_buf[sc_e1_count++] = raw;
        if (sc_e1_count < 2)
            return 0;
        /* Two-byte E1 sequence complete */
        sc_prefix   = 0;
        sc_e1_count = 0;
        /* Pause key – produce a single synthetic press, no release byte */
        ev_out->keycode  = KEY_PAUSE;
        ev_out->pressed  = 1;
        ev_out->ascii    = 0;
        ev_out->modifiers = sc_modifiers;
        return 1;
    }

    /* ── Decode press / release ──────────────────────────────────────── */
    int      pressed  = !(raw & SC_RELEASE);
    uint8_t  scancode = raw & ~SC_RELEASE;

    uint8_t keycode = 0;
    if (sc_prefix == SC_PREFIX_E0) {
        if (scancode < 128)
            keycode = sc_e0_to_key[scancode];
        sc_prefix = 0;
    } else {
        if (scancode < 128)
            keycode = sc_to_key[scancode];
    }

    if (keycode == 0)
        return 0;   /* unknown scan code */

    update_modifiers(keycode, pressed);

    ev_out->keycode   = keycode;
    ev_out->pressed   = pressed;
    ev_out->modifiers = sc_modifiers;
    ev_out->ascii     = pressed ? derive_ascii(keycode) : 0;

    return 1;
}

/*
 * scancode_irq_handler() - call from your IRQ1 vector.
 *
 * Reads all pending bytes from the PS/2 data port, decodes them, and pushes
 * completed events to the keyboard subsystem.  Sends EOI before returning.
 */
void scancode_irq_handler(void)
{
    struct key_event ev;

    /* Drain the PS/2 output buffer (may have more than one byte ready). */
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_OBF) {
        uint8_t raw = inb(PS2_DATA_PORT);
        if (scancode_translate(raw, &ev))
            kbd_push(&ev);
    }

    /* Send End-Of-Interrupt to the master PIC. */
    outb(PIC1_CMD, PIC_EOI);
}
