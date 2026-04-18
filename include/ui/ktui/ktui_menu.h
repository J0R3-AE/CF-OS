#pragma once

#include "ktui_widget.h"

typedef void (*ktui_menu_action_t)(void);

typedef struct {
    const char *label;
    const char *desc;   // 👈 NEW
    ktui_menu_action_t action;
} ktui_menu_item_t;

typedef struct {
    ktui_widget_t base;

    ktui_menu_item_t *items;
    int count;
    int selected;

} ktui_menu_t;

void ktui_menu_render(ktui_widget_t *w);
void ktui_menu_input(ktui_widget_t *w, int key);