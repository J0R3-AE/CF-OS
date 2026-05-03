#pragma once
/**
 * @file keyboard.h
 * @brief PS/2 keyboard driver interface.
 *
 * Provides:
 * - keyboard initialization
 * - IRQ handler
 * - character buffering
 * - simple non‑blocking input checks
 *
 * Arrow keys are exposed as extended keycodes above 0x80.
 */

#include <stdint.h>
#include "arch/idt.h"

/* -------------------------------------------------------------------------- */
/*  Special Keycodes                                                          */
/* -------------------------------------------------------------------------- */

/* printable */
#define KEY_NONE        0
#define KEY_BACKSPACE   8
#define KEY_TAB         9
#define KEY_ENTER       10
#define KEY_ESC         27

/* arrow keys (logical input layer) */
#define KEY_UP          1001
#define KEY_DOWN        1002
#define KEY_LEFT        1003
#define KEY_RIGHT       1004

/* -------------------------------------------------------------------------- */
/*  Keyboard Driver API                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the keyboard driver.
 *
 * Sets up the PS/2 controller, enables IRQ1, and prepares the internal
 * character buffer. Must be called during early kernel initialization.
 */
void keyboard_init(void);

/**
 * @brief Keyboard IRQ handler (IRQ1).
 *
 * Reads the scancode from port 0x60, translates it if needed, and pushes
 * characters into the internal buffer. Called automatically by the IRQ
 * dispatch system.
 */
void keyboard_irq_handler(struct registers *r);

/**
 * @brief Check if a character is available.
 *
 * @return 1 if a character is waiting, 0 otherwise.
 */
int keyboard_has_char(void);

/**
 * @brief Retrieve the next character from the keyboard buffer.
 *
 * Blocks until a character is available (depending on your implementation),
 * or returns immediately if using a non‑blocking buffer.
 *
 * @return ASCII character or extended keycode (e.g., KBD_ARROW_UP).
 */
int keyboard_getchar(void);

/* DRIVERS_KEYBOARD_H */
