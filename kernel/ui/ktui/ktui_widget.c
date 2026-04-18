#include "ui/ktui/ktui_widget.h"

static void ktui_widget_default_render(struct ktui_widget *self)
{
    (void)self;
}

static void ktui_widget_default_input(struct ktui_widget *self, int key)
{
    (void)self;
    (void)key;
}

void ktui_widget_init(ktui_widget_t *w,
                      int x, int y,
                      void (*render)(struct ktui_widget *),
                      void (*input)(struct ktui_widget *, int key),
                      int selectable)
{
    if (!w)
        return;

    w->x = x;
    w->y = y;
    w->render = render ? render : ktui_widget_default_render;
    w->input = input ? input : ktui_widget_default_input;
    w->selectable = selectable;
}