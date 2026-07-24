#ifndef LINE_H
#define LINE_H

#include <stdint.h>
#include "keyevent.h"

void line_reset(void);
int  line_handle_keyevent(const key_event_t *ev);
const char *line_get_buffer(void);

#endif
