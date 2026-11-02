#include "kernel/drivers/keymap.h"

u8 keymap_translate_ascii(keycode_t key, bool shift, bool caps_lock)
{
    static const u8 letters[KEY_MAX] = {
        [KEY_A] = 'a', [KEY_B] = 'b', [KEY_C] = 'c', [KEY_D] = 'd',
        [KEY_E] = 'e', [KEY_F] = 'f', [KEY_G] = 'g', [KEY_H] = 'h',
        [KEY_I] = 'i', [KEY_J] = 'j', [KEY_K] = 'k', [KEY_L] = 'l',
        [KEY_M] = 'm', [KEY_N] = 'n', [KEY_O] = 'o', [KEY_P] = 'p',
        [KEY_Q] = 'q', [KEY_R] = 'r', [KEY_S] = 's', [KEY_T] = 't',
        [KEY_U] = 'u', [KEY_V] = 'v', [KEY_W] = 'w', [KEY_X] = 'x',
        [KEY_Y] = 'y', [KEY_Z] = 'z'
    };
    static const char digits[] = "1234567890";
    static const char shifted_digits[] = "!@#$%^&*()";
    bool uppercase = shift != caps_lock;
    u8 letter;

    if (key <= KEY_NONE || key >= KEY_MAX)
        return 0;

    letter = letters[key];
    if (letter)
    {
        return (u8)(uppercase ? letter - 'a' + 'A' : letter);
    }

    if (key >= KEY_1 && key <= KEY_0)
    {
        usize index = (key == KEY_0) ? 9 : (usize)(key - KEY_1);
        return (u8)(shift ? shifted_digits[index] : digits[index]);
    }

    switch (key)
    {
    case KEY_MINUS: return shift ? '_' : '-';
    case KEY_EQUALS: return shift ? '+' : '=';
    case KEY_LEFT_BRACKET: return shift ? '{' : '[';
    case KEY_RIGHT_BRACKET: return shift ? '}' : ']';
    case KEY_BACKSLASH: return shift ? '|' : '\\';
    case KEY_SEMICOLON: return shift ? ':' : ';';
    case KEY_APOSTROPHE: return shift ? '"' : '\'';
    case KEY_GRAVE: return shift ? '~' : '`';
    case KEY_COMMA: return shift ? '<' : ',';
    case KEY_PERIOD: return shift ? '>' : '.';
    case KEY_SLASH: return shift ? '?' : '/';
    case KEY_SPACE: return ' ';
    case KEY_ENTER: return '\n';
    case KEY_BACKSPACE: return '\b';
    case KEY_TAB: return '\t';
    default: return 0;
    }
}
