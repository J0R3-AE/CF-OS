#include "kernel/drivers/vga.h"
#include "kernel/arch/io.h"

#define VGA_MEMORY ((volatile u16 *)0xB8000)
#define VGA_CONTROL 0x3D4
#define VGA_DATA 0x3D5

static u16 vga_entry(char character, u8 foreground, u8 background)
{
    u8 color = (u8)((background << 4) | (foreground & 0x0F));
    return (u16)(u8)character | ((u16)color << 8);
}

void vga_text_init(void)
{
    vga_text_clear(VGA_LIGHT_GREY, VGA_BLACK);
    vga_text_set_cursor(0, 0);
}

void vga_text_clear(u8 foreground, u8 background)
{
    u16 blank = vga_entry(' ', foreground, background);

    for (usize i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_MEMORY[i] = blank;
}

void vga_text_put_cell(usize row, usize col, char character,
                       u8 foreground, u8 background)
{
    if (row >= VGA_HEIGHT || col >= VGA_WIDTH)
        return;
    VGA_MEMORY[row * VGA_WIDTH + col] =
        vga_entry(character, foreground, background);
}

void vga_text_scroll(u8 foreground, u8 background)
{
    for (usize row = 1; row < VGA_HEIGHT; row++)
        for (usize col = 0; col < VGA_WIDTH; col++)
            VGA_MEMORY[(row - 1) * VGA_WIDTH + col] =
                VGA_MEMORY[row * VGA_WIDTH + col];

    for (usize col = 0; col < VGA_WIDTH; col++)
        vga_text_put_cell(VGA_HEIGHT - 1, col, ' ', foreground, background);
}

void vga_text_set_cursor(usize row, usize col)
{
    u16 position;

    if (row >= VGA_HEIGHT)
        row = VGA_HEIGHT - 1;
    if (col >= VGA_WIDTH)
        col = VGA_WIDTH - 1;

    position = (u16)(row * VGA_WIDTH + col);
    out8(VGA_CONTROL, 0x0F);
    out8(VGA_DATA, (u8)position);
    out8(VGA_CONTROL, 0x0E);
    out8(VGA_DATA, (u8)(position >> 8));
}
