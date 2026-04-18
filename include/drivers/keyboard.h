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

/* -------------------------------------------------------------------------- */
/*  Special Keycodes                                                          */
/* -------------------------------------------------------------------------- */

#define KBD_ARROW_UP 0x80
#define KBD_ARROW_DOWN 0x81
#define KBD_ARROW_LEFT 0x82
#define KBD_ARROW_RIGHT 0x83
#define KEY_ENTER  0x100
#define KEY_ESC    0x101
#define KEY_UP     0x102
#define KEY_DOWN   0x103

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
void keyboard_irq_handler(void);

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
