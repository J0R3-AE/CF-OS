#include "ui/ktui/ktui_screen.h"
#include "ui/ktui/ktui_widget.h"
#include "drivers/tty.h"
#include "libk/printf.h"
#include "libk/types.h"

#define KEY_TAB 9

static size_t screen_next_selectable(ktui_screen_t *s, size_t start)
{
    if (!s || !s->widgets || s->widget_count == 0)
        return 0;

    for (size_t i = 0; i < s->widget_count; i++)
    {
        size_t idx = (start + i) % s->widget_count;
        if (s->widgets[idx] && s->widgets[idx]->selectable)
            return idx;
    }

    return start;
}

void ktui_screen_render(ktui_screen_t *s)
{
    if (!s)
        return;

    TTY_setcolor(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    TTY_clear();

    TTY_setpos(0, 2);
    printf("%s", s->title ? s->title : "");

    TTY_setpos(1, 2);
    printf("------------------------------------------------------------");

    if (!s->widgets)
        return;

    for (size_t i = 0; i < s->widget_count; i++)
    {
        ktui_widget_t *w = s->widgets[i];
        if (!w || !w->render)
            continue;

        w->render(w);
    }
}

void ktui_screen_input(ktui_screen_t *s, int key)
{
    if (!s || !s->widgets || s->widget_count == 0)
        return;

    if (key == KEY_TAB)
    {
        s->selected = screen_next_selectable(s, s->selected + 1);
        return;
    }

    if (s->selected >= s->widget_count)
        s->selected = screen_next_selectable(s, 0);

    ktui_widget_t *w = s->widgets[s->selected];
    if (w && w->input)
        w->input(w, key);
}