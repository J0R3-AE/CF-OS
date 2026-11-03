#ifndef KERNEL_DRIVERS_CONSOLE_H
#define KERNEL_DRIVERS_CONSOLE_H

#include "libc/types.h"

void console_init_vga(void);
bool console_init_framebuffer(uptr address, u32 width, u32 height,
                              u32 pitch, u32 bpp);
void console_clear(void);
void console_clearline(usize row);
void console_putc(char c);
void console_puts(const char *text);
void console_backspace(void);
void console_scroll(void);
void console_putc_at(usize row, usize col, char c, u8 foreground,
                     u8 background);
void console_set_color(u8 foreground, u8 background);
void console_get_color(u8 *foreground, u8 *background);
void console_get_cursor(usize *row, usize *col);
void console_set_cursor(usize row, usize col);
usize console_rows(void);
usize console_cols(void);
#endif
