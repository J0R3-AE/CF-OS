#include "ui/ktui/ktui_menu.h"
#include "drivers/tty.h"
#include "libk/printf.h"
#include "libk/types.h"

#define KEY_ENTER 10
#define KEY_ESC 27
#define KEY_UP 1001
#define KEY_DOWN 1002

static void menu_clamp_selected(ktui_menu_t *m)
{
    if (!m || m->count <= 0)
    {
        m->selected = 0;
        return;
    }

    if (m->selected < 0)
        m->selected = 0;
    if (m->selected >= m->count)
        m->selected = m->count - 1;
}

void ktui_menu_render(ktui_widget_t *w)
{
    if (!w)
        return;

    ktui_menu_t *m = (ktui_menu_t *)w;
    menu_clamp_selected(m);

    /* =========================
       Description Bar (TOP)
       ========================= */
    TTY_setcolor(TTY_COLOR_BLACK, TTY_COLOR_LIGHT_GREY);
    TTY_setpos(2, 2);

    const char *desc = "";
    if (m->selected >= 0 && m->selected < m->count)
    {
        desc = m->items[m->selected].desc ? m->items[m->selected].desc : "";
    }

    printf("%-60s", desc); /* padded line */

    /* =========================
       Menu Items
       ========================= */
    for (int i = 0; i < m->count; i++)
    {
        TTY_setpos(w->y + i, w->x);

        if (i == m->selected)
        {
            TTY_setcolor(TTY_COLOR_BLACK, TTY_COLOR_LIGHT_GREY);
            printf("> %s",
                   m->items[i].label ? m->items[i].label : "");
        }
        else
        {
            TTY_setcolor(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
            printf("  %s",
                   m->items[i].label ? m->items[i].label : "");
        }
    }

    /* =========================
       Footer / Help Bar
       ========================= */
    size_t rows, cols;
    TTY_getsize(&rows, &cols);

    TTY_setcolor(TTY_COLOR_BLACK, TTY_COLOR_LIGHT_GREY);
    TTY_setpos(rows - 1, 0);

    printf(" ↑↓ Navigate  ENTER Select  ESC Back ");

    TTY_resetcolor();
}

void ktui_menu_input(ktui_widget_t *w, int key)
{
    if (!w)
        return;

    ktui_menu_t *m = (ktui_menu_t *)w;
    if (!m || m->count <= 0)
        return;

    if (key == KEY_UP || key == 'w' || key == 'W')
    {
        if (m->selected > 0)
            m->selected--;
        else
            m->selected = m->count - 1;
        return;
    }

    if (key == KEY_DOWN || key == 's' || key == 'S')
    {
        if (m->selected + 1 < m->count)
            m->selected++;
        else
            m->selected = 0;
        return;
    }

    if (key == KEY_ENTER || key == '\n' || key == '\r')
    {
        if (m->selected >= 0 && m->selected < m->count)
        {
            if (m->items[m->selected].action)
                m->items[m->selected].action();
        }
        return;
    }

    if (key == KEY_ESC || key == 27)
    {
        return;
    }
}