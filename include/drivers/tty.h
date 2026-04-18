#pragma once
#ifndef TTY_H
#define TTY_H

#include "libk/types.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @def VGA_MEMORY
 * @brief Physical address of the VGA text‑mode buffer.
 */
#define VGA_MEMORY 0xB8000

/**
 * @name VGA I/O Ports
 * @{
 */
#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5
/** @} */

/**
 * @name VGA Cursor Registers
 * @{
 */
#define VGA_CURSOR_START 0x0A
#define VGA_CURSOR_END 0x0B
#define VGA_CURSOR_HIGH 0x0E
#define VGA_CURSOR_LOW 0x0F
/** @} */

/**
 * @enum TTY_color
 * @brief Standard 16‑color VGA palette used by the TTY.
 */
enum TTY_color
{
   TTY_COLOR_BLACK = 0,
   TTY_COLOR_BLUE,
   TTY_COLOR_GREEN,
   TTY_COLOR_CYAN,
   TTY_COLOR_RED,
   TTY_COLOR_MAGENTA,
   TTY_COLOR_BROWN,
   TTY_COLOR_LIGHT_GREY,
   TTY_COLOR_DARK_GREY,
   TTY_COLOR_LIGHT_BLUE,
   TTY_COLOR_LIGHT_GREEN,
   TTY_COLOR_LIGHT_CYAN,
   TTY_COLOR_LIGHT_RED,
   TTY_COLOR_LIGHT_MAGENTA,
   TTY_COLOR_YELLOW,
   TTY_COLOR_WHITE
};

/**
 * @name Global TTY State
 * @brief These variables are defined in tty.c and represent the active TTY state.
 * @{
 */
extern volatile uint16_t *TTY_base; /**< Base address of VGA text buffer (VGA mode only). */
extern size_t TTY_row;              /**< Current cursor row. */
extern size_t TTY_col;              /**< Current cursor column. */
extern size_t TTY_ROWS;             /**< Number of visible rows (dynamic in FB mode). */
extern size_t TTY_COLS;             /**< Number of visible columns (dynamic in FB mode). */
extern uint8_t TTY_fg;              /**< Current foreground color. */
extern uint8_t TTY_bg;              /**< Current background color. */
/** @} */

/**
 * @brief Initialize the TTY subsystem.
 *
 * Clears the screen, resets the cursor, and configures VGA or framebuffer
 * mode depending on the active backend.
 */
void TTY_init(void);

/**
 * @brief Set the VGA text buffer base address.
 * @param addr New base address.
 */
void TTY_set_TTY_base(volatile uint16_t *addr);

/** @brief Get the current cursor row. */
size_t TTY_getrow(void);

/** @brief Set the current cursor row (clamped). */
void TTY_setrow(size_t row);

/** @brief Get the current cursor column. */
size_t TTY_getcol(void);

/** @brief Set the current cursor column (clamped). */
void TTY_setcol(size_t col);

/**
 * @brief Get the current cursor position.
 * @param row Output row pointer.
 * @param col Output column pointer.
 */
void TTY_getpos(size_t *row, size_t *col);

/**
 * @brief Set the cursor position (clamped).
 */
void TTY_setpos(size_t row, size_t col);

/**
 * @brief Get the TTY screen size.
 * @param rows Output number of rows.
 * @param cols Output number of columns.
 */
void TTY_getsize(size_t *rows, size_t *cols);

/**
 * @brief Set the current text color.
 * @param fg Foreground color.
 * @param bg Background color.
 */
void TTY_setcolor(uint8_t fg, uint8_t bg);

/**
 * @brief Get the current text color.
 */
void TTY_getcolor(uint8_t *fg, uint8_t *bg);

/** @brief Reset text color to white‑on‑black. */
void TTY_resetcolor(void);

/** @brief Clear the entire screen. */
void TTY_clear(void);

/** @brief Clear a specific line. */
void TTY_clearline(size_t row);

/** @brief Print a single character at the cursor. */
void TTY_putc(char c);

/** @brief Print a null‑terminated string. */
void TTY_puts(const char *s);

/** @brief Perform a backspace operation. */
void TTY_backspace(void);

/** @brief Scroll the screen up by one line. */
void TTY_scroll(void);

/**
 * @brief Draw a character at a specific position without moving the cursor.
 */
void TTY_putcat(size_t row, size_t col, char ch, uint8_t fg, uint8_t bg);

/**
 * @brief Draw a string at a specific position without moving the cursor.
 */
void TTY_putsat(size_t row, size_t col, const char *s, uint8_t fg, uint8_t bg);

/**
 * @brief Enable or disable the framebuffer backend.
 * @param enable 1 = framebuffer, 0 = VGA text mode.
 */
void TTY_set_fb_backend(int enable);

#endif
