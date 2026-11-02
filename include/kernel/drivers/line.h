#ifndef LINE_H
#define LINE_H

#include "libc/types.h"
#include "keyevent.h"

#define LINE_INPUT_CAPACITY 256

void line_reset(void);
int line_handle_keyevent(const key_event_t *event);
int line_ready(void);
int line_eof(void);
const char *line_get_buffer(void);
usize line_get_length(void);
void line_consume(void);
int line_read(char *buffer, usize capacity, bool wait);
int line_scan(char *buffer, usize capacity);
void line_handle_ascii(u8 character);

#endif
