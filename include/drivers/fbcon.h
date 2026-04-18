#pragma once
/**
 * @file fbcon.h
 * @brief Framebuffer text console backend.
 *
 * Provides a simple text console rendered directly into a linear framebuffer.
 * Supports:
 * - character and string output
 * - scrolling
 * - clearing screen or individual lines
 * - direct cell writes with foreground/background colors
 *
 * This backend is typically used by the kernel TTY layer.
 */

#include "libk/types.h"
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/*  Console Geometry                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Get number of text columns.
 *
 * @return Number of character columns available.
 */
size_t fbcon_cols(void);

/**
 * @brief Get number of text rows.
 *
 * @return Number of character rows available.
 */
size_t fbcon_rows(void);

/* -------------------------------------------------------------------------- */
/*  Initialization                                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the framebuffer console.
 *
 * @param phys_addr Physical address of the framebuffer.
 * @param width     Pixel width.
 * @param height    Pixel height.
 * @param pitch     Bytes per scanline.
 * @param bpp       Bits per pixel.
 */
void fbcon_init(u32 phys_addr, u32 width, u32 height, u32 pitch, u32 bpp);

/* -------------------------------------------------------------------------- */
/*  Basic Output                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Clear the entire console.
 */
void fbcon_clear(void);

/**
 * @brief Output a single character at the current cursor position.
 *
 * Handles newline, scrolling, and cursor movement.
 *
 * @param c Character to print.
 */
void fbcon_putc(char c);

/**
 * @brief Output a null‑terminated string.
 *
 * @param s String to print.
 */
void fbcon_puts(const char *s);

/**
 * @brief Write a raw buffer of characters.
 *
 * @param s   Character buffer.
 * @param len Number of characters to write.
 */
void fbcon_write(const char *s, size_t len);

/* -------------------------------------------------------------------------- */
/*  TTY‑Style Helpers                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Clear a specific text row.
 *
 * @param row Row index to clear.
 */
void fbcon_clearline(size_t row);

/**
 * @brief Write a character at a specific row/column with colors.
 *
 * @param row Row index.
 * @param col Column index.
 * @param ch  Character to draw.
 * @param fg  Foreground color (RGB or packed pixel format).
 * @param bg  Background color.
 */
void fbcon_putcat(size_t row, size_t col, char ch, u32 fg, u32 bg);

/**
 * @brief Scroll the console up by one row.
 */
void fbcon_scroll(void);
