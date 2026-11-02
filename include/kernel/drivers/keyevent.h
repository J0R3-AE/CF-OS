#ifndef KEYEVENT_H
#define KEYEVENT_H

#include "libc/types.h"
#include "keycodes.h"

typedef enum {
    KEY_EVENT_PRESS,
    KEY_EVENT_RELEASE
} key_event_type_t;

typedef struct {
    key_event_type_t type;
    keycode_t        keycode;
    u8                ascii;
} key_event_t;

#endif
