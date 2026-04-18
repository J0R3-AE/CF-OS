#include "ui/ktui/ktui.h"
#include "ui/ktui/ktui_screen.h"
#include "drivers/kbd.h"
#include "drivers/tty.h"
#include "libk/printf.h"
#include "libk/types.h"

static ktui_screen_t *g_active_screen = NULL;
static int g_dirty = 1;

void ktui_init(void)
{
    g_active_screen = NULL;
    g_dirty = 1;
}

void ktui_set_screen(ktui_screen_t *screen)
{
    if (g_active_screen && g_active_screen->on_exit)
        g_active_screen->on_exit();

    g_active_screen = screen;
    g_dirty = 1;

    if (g_active_screen && g_active_screen->on_enter)
        g_active_screen->on_enter();
}

static void ktui_render_active(void)
{
    if (!g_active_screen)
        return;

    ktui_screen_render(g_active_screen);
}

void ktui_run(void)
{
    while (1)
    {
        if (g_dirty)
        {
            ktui_render_active();
            g_dirty = 0;
        }

        int key = kbd_try_getchar();
        if (key < 0)
        {
            asm volatile("hlt");
            continue;
        }

        if (!g_active_screen)
            continue;

        ktui_screen_input(g_active_screen, key);
        g_dirty = 1;
    }
}