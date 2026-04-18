#pragma once

#include <stddef.h>

struct ktui_widget;

typedef struct ktui_screen {
    const char *title;

    struct ktui_widget **widgets;
    size_t widget_count;

    size_t selected;

    void (*on_enter)(void);
    void (*on_exit)(void);

} ktui_screen_t;

void ktui_screen_render(ktui_screen_t *s);
void ktui_screen_input(ktui_screen_t *s, int key);