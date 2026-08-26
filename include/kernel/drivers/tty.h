#pragma once

#ifndef TTY_H
#define TTY_H

#include "libc/types.h"

#include <stdint.h>
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* VGA                                                                         */
/* -------------------------------------------------------------------------- */

#define VGA_MEMORY 0xB8000

#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5

#define VGA_CURSOR_START 0x0A
#define VGA_CURSOR_END   0x0B
#define VGA_CURSOR_HIGH  0x0E
#define VGA_CURSOR_LOW   0x0F


/* -------------------------------------------------------------------------- */
/* TTY colors                                                                  */
/* -------------------------------------------------------------------------- */

typedef enum
{
    TTY_COLOR_BLACK = 0,
    TTY_COLOR_RED,
    TTY_COLOR_GREEN,
    TTY_COLOR_YELLOW,
    TTY_COLOR_BLUE,
    TTY_COLOR_MAGENTA,
    TTY_COLOR_CYAN,
    TTY_COLOR_WHITE,

    TTY_COLOR_BRIGHT_BLACK,
    TTY_COLOR_BRIGHT_RED,
    TTY_COLOR_BRIGHT_GREEN,
    TTY_COLOR_BRIGHT_YELLOW,
    TTY_COLOR_BRIGHT_BLUE,
    TTY_COLOR_BRIGHT_MAGENTA,
    TTY_COLOR_BRIGHT_CYAN,
    TTY_COLOR_BRIGHT_WHITE

} tty_color_t;


/* -------------------------------------------------------------------------- */
/* Global TTY state                                                            */
/* -------------------------------------------------------------------------- */

extern volatile uint16_t *TTY_base;

extern size_t TTY_row;
extern size_t TTY_col;

extern size_t TTY_ROWS;
extern size_t TTY_COLS;

extern uint8_t TTY_fg;
extern uint8_t TTY_bg;


/* -------------------------------------------------------------------------- */
/* Initialization                                                              */
/* -------------------------------------------------------------------------- */

void TTY_init(void);

void TTY_set_TTY_base(
    volatile uint16_t *addr);

void TTY_set_fb_backend(
    int enable);


/* -------------------------------------------------------------------------- */
/* Cursor                                                                      */
/* -------------------------------------------------------------------------- */

size_t TTY_getrow(void);
void TTY_setrow(size_t row);

size_t TTY_getcol(void);
void TTY_setcol(size_t col);

void TTY_getpos(
    size_t *row,
    size_t *col);

void TTY_setpos(
    size_t row,
    size_t col);

void TTY_getsize(
    size_t *rows,
    size_t *cols);


/* -------------------------------------------------------------------------- */
/* Colors                                                                      */
/* -------------------------------------------------------------------------- */

void TTY_setcolor(
    uint8_t fg,
    uint8_t bg);

void TTY_getcolor(
    uint8_t *fg,
    uint8_t *bg);

void TTY_resetcolor(void);


/* -------------------------------------------------------------------------- */
/* Screen                                                                      */
/* -------------------------------------------------------------------------- */

void TTY_clear(void);

void TTY_clearline(size_t row);

void TTY_scroll(void);


/* -------------------------------------------------------------------------- */
/* Output                                                                      */
/* -------------------------------------------------------------------------- */

void TTY_putc(char c);

void TTY_puts(const char *s);

void TTY_backspace(void);

void TTY_putcat(
    size_t row,
    size_t col,
    char ch,
    uint8_t fg,
    uint8_t bg);

void TTY_putsat(
    size_t row,
    size_t col,
    const char *s,
    uint8_t fg,
    uint8_t bg);


/* -------------------------------------------------------------------------- */
/* Input                                                                       */
/* -------------------------------------------------------------------------- */

/*
 * Returns non-zero when a complete input line is available.
 */
int TTY_input_ready(void);

/*
 * Returns non-zero when EOF has been generated.
 */
int TTY_input_eof(void);

/*
 * Read the completed canonical input line.
 *
 * Returns the number of bytes copied, not including the terminating '\0'.
 */
size_t TTY_readline(
    char *buffer,
    size_t size);

/*
 * Consume the current completed input line.
 */
void TTY_input_consume(void);

#endif