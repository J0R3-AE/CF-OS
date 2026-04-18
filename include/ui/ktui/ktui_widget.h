#pragma once

typedef struct ktui_widget {
    int x, y;

    void (*render)(struct ktui_widget *);
    void (*input)(struct ktui_widget *, int key);

    int selectable;

} ktui_widget_t;