/*
 * scancode.h - public interface of the PS/2 keyboard hardware driver
 */

#ifndef SCANCODE_H
#define SCANCODE_H

#include <stdint.h>

/* ── Generic key codes ────────────────────────────────────────────────────── */

typedef enum {
    KEY_NONE = 0,

    /* Letters */
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H,
    KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P,
    KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X,
    KEY_Y, KEY_Z,

    /* Digits (top row) */
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

    /* Punctuation */
    KEY_MINUS, KEY_EQUALS,
    KEY_LBRACKET, KEY_RBRACKET,
    KEY_SEMICOLON, KEY_APOSTROPHE,
    KEY_GRAVE, KEY_BACKSLASH,
    KEY_COMMA, KEY_DOT, KEY_SLASH,

    /* Whitespace / control */
    KEY_SPACE, KEY_TAB, KEY_ENTER, KEY_BACKSPACE, KEY_ESCAPE, KEY_DELETE,

    /* Modifiers */
    KEY_LSHIFT, KEY_RSHIFT,
    KEY_LCTRL,  KEY_RCTRL,
    KEY_LALT,   KEY_RALT,
    KEY_LSUPER, KEY_RSUPER,
    KEY_MENU,

    /* Locks */
    KEY_CAPS_LOCK, KEY_NUM_LOCK, KEY_SCROLL_LOCK,

    /* Function keys */
    KEY_F1,  KEY_F2,  KEY_F3,  KEY_F4,  KEY_F5,  KEY_F6,
    KEY_F7,  KEY_F8,  KEY_F9,  KEY_F10, KEY_F11, KEY_F12,

    /* Navigation */
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_HOME, KEY_END,
    KEY_PAGE_UP, KEY_PAGE_DOWN,
    KEY_INSERT, KEY_PAUSE,

    /* Keypad */
    KEY_KP_0, KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_4,
    KEY_KP_5, KEY_KP_6, KEY_KP_7, KEY_KP_8, KEY_KP_9,
    KEY_KP_DOT, KEY_KP_ENTER, KEY_KP_PLUS, KEY_KP_MINUS,
    KEY_KP_STAR, KEY_KP_SLASH,

    KEY_COUNT   /* sentinel – keep last */
} keycode_t;

/* ── Modifier bitmask (same bits used in sc_modifiers) ───────────────────── */

#define KEY_MOD_SHIFT      (1 << 0)
#define KEY_MOD_CTRL       (1 << 1)
#define KEY_MOD_ALT        (1 << 2)
#define KEY_MOD_CAPS_LOCK  (1 << 3)
#define KEY_MOD_NUM_LOCK   (1 << 4)
#define KEY_MOD_SCROLL_LOCK (1 << 5)

/* ── Keyboard event ───────────────────────────────────────────────────────── */

struct key_event {
    uint8_t  keycode;   /* one of keycode_t          */
    uint8_t  pressed;   /* 1 = press, 0 = release    */
    uint8_t  modifiers; /* KEY_MOD_* bitmask          */
    char     ascii;     /* printable char, or 0       */
};

/* ── Driver API ───────────────────────────────────────────────────────────── */

void scancode_init(void);
void scancode_irq_handler(void);
int  scancode_translate(uint8_t raw, struct key_event *ev_out);

#endif /* SCANCODE_H */
