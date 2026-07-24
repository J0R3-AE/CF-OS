#pragma once
#include <stdint.h>
#include <stddef.h>

/* init */
void console_init(uint32_t fb_addr, uint32_t w, uint32_t h,
                  uint32_t pitch, uint32_t bpp);

/* screen */
void console_clear(void);

/* output */
void console_putc(char c);
void console_puts(const char *s);

/* direct draw */
void console_putc_at(size_t row, size_t col, char c,
                     uint32_t fg, uint32_t bg);

/* utilities */
void console_clearline(size_t row);
void console_scroll(void);

/* size */
size_t console_rows(void);
size_t console_cols(void);