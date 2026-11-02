#include "kernel/drivers/console.h"
#include "kernel/drivers/framebuffer.h"
#include "kernel/drivers/vga.h"
#include "gui/font.h"

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

typedef enum
{
    CONSOLE_VGA,
    CONSOLE_FRAMEBUFFER
} console_backend_t;

static console_backend_t backend = CONSOLE_VGA;
static usize cursor_row;
static usize cursor_col;
static usize rows = VGA_HEIGHT;
static usize cols = VGA_WIDTH;
static u8 foreground = VGA_LIGHT_GREY;
static u8 background = VGA_BLACK;

static u32 palette_color(u8 color)
{
    static const u32 palette[16] = {
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
        0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
        0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
    };
    return palette[color & 0x0F];
}

static void draw_framebuffer_cell(usize row, usize col, char character,
                                  u8 fg, u8 bg)
{
    const u8 *glyph = font8x16[(u8)character];
    u32 fg_color = palette_color(fg);
    u32 bg_color = palette_color(bg);

    for (u32 y = 0; y < FONT_HEIGHT; y++)
    {
        for (u32 x = 0; x < FONT_WIDTH; x++)
        {
            u32 color = (glyph[y] & (0x80 >> x)) ? fg_color : bg_color;
            framebuffer_putpixel((int)(col * FONT_WIDTH + x),
                                 (int)(row * FONT_HEIGHT + y), color);
        }
    }
}

static void render_cell(usize row, usize col, char character, u8 fg, u8 bg)
{
    if (backend == CONSOLE_FRAMEBUFFER)
        draw_framebuffer_cell(row, col, character, fg, bg);
    else
        vga_text_put_cell(row, col, character, fg, bg);
}

static void update_cursor(void)
{
    if (backend == CONSOLE_VGA)
        vga_text_set_cursor(cursor_row, cursor_col);
}

void console_init_vga(void)
{
    backend = CONSOLE_VGA;
    rows = VGA_HEIGHT;
    cols = VGA_WIDTH;
    cursor_row = 0;
    cursor_col = 0;
    foreground = VGA_LIGHT_GREY;
    background = VGA_BLACK;
    vga_text_init();
}

bool console_init_framebuffer(uptr address, u32 width, u32 height,
                              u32 pitch, u32 bpp)
{
    usize new_cols, new_rows;

    if (width < FONT_WIDTH || height < FONT_HEIGHT)
        return false;
    if (!framebuffer_init(address, width, height, pitch, bpp))
        return false;

    new_cols = width / FONT_WIDTH;
    new_rows = height / FONT_HEIGHT;
    if (!new_cols || !new_rows)
        return false;

    backend = CONSOLE_FRAMEBUFFER;
    cols = new_cols;
    rows = new_rows;
    cursor_row = 0;
    cursor_col = 0;
    foreground = VGA_LIGHT_GREY;
    background = VGA_BLACK;
    framebuffer_fill(palette_color(background));
    return true;
}

usize console_rows(void) { return rows; }
usize console_cols(void) { return cols; }

void console_set_color(u8 fg, u8 bg)
{
    foreground = fg & 0x0F;
    background = bg & 0x0F;
}

void console_get_cursor(usize *row, usize *col)
{
    if (row)
        *row = cursor_row;
    if (col)
        *col = cursor_col;
}

void console_set_cursor(usize row, usize col)
{
    cursor_row = row < rows ? row : rows - 1;
    cursor_col = col < cols ? col : cols - 1;
    update_cursor();
}

void console_putc_at(usize row, usize col, char character, u8 fg, u8 bg)
{
    if (row < rows && col < cols)
        render_cell(row, col, character, fg, bg);
}

void console_clear(void)
{
    cursor_row = 0;
    cursor_col = 0;
    if (backend == CONSOLE_FRAMEBUFFER)
        framebuffer_fill(palette_color(background));
    else
        vga_text_clear(foreground, background);
    update_cursor();
}

void console_clearline(usize row)
{
    if (row >= rows)
        return;

    for (usize col = 0; col < cols; col++)
        render_cell(row, col, ' ', foreground, background);
}

void console_scroll(void)
{
    if (backend == CONSOLE_FRAMEBUFFER)
        framebuffer_scroll(FONT_HEIGHT, palette_color(background));
    else
        vga_text_scroll(foreground, background);

    if (cursor_row)
        cursor_row--;
    update_cursor();
}

static void advance_line(void)
{
    cursor_col = 0;
    cursor_row++;
    if (cursor_row >= rows)
    {
        console_scroll();
        cursor_row = rows - 1;
    }
    update_cursor();
}

void console_backspace(void)
{
    if (cursor_col)
        cursor_col--;
    else if (cursor_row)
    {
        cursor_row--;
        cursor_col = cols - 1;
    }
    else
        return;

    render_cell(cursor_row, cursor_col, ' ', foreground, background);
    update_cursor();
}

void console_putc(char character)
{
    if (character == '\n')
    {
        advance_line();
        return;
    }
    if (character == '\r')
    {
        cursor_col = 0;
        update_cursor();
        return;
    }
    if (character == '\b')
    {
        console_backspace();
        return;
    }
    if (character == '\t')
    {
        usize spaces = 4 - (cursor_col % 4);
        while (spaces--)
            console_putc(' ');
        return;
    }
    if ((u8)character < 32)
        return;

    render_cell(cursor_row, cursor_col, character, foreground, background);
    cursor_col++;
    if (cursor_col >= cols)
        advance_line();
    else
        update_cursor();
}

void console_puts(const char *text)
{
    if (!text)
        return;
    while (*text)
        console_putc(*text++);
}
