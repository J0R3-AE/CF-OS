#ifndef KEYEVENT_H
#define KEYEVENT_H

#include <stdint.h>
#include "keycodes.h"

typedef enum {
    KEY_EVENT_PRESS,
    KEY_EVENT_RELEASE
} key_event_type_t;

typedef struct {
    key_event_type_t type;
    keycode_t        keycode;
    uint8_t          ascii;     // 0 if non-printable
} key_event_t;

#endif
