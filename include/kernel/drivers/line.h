#pragma once

#include <stddef.h>

#include "kernel/drivers/keyevent.h"

void line_reset(void);

int line_handle_keyevent(const key_event_t *ev);

int line_ready(void);
int line_eof(void);

const char *line_get_buffer(void);
size_t line_get_length(void);

void line_consume(void);