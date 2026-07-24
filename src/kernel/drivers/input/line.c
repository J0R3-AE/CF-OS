#include <stdint.h>
#include "kernel/drivers/keyevent.h"
#include "kernel/drivers/keymap.h"   // or declare keymap_get_ascii in a header

#define LINE_BUFFER_SIZE 256

static char line_buffer[LINE_BUFFER_SIZE];
static uint32_t line_len = 0;

void line_reset(void)
{
    line_len = 0;
}

int line_handle_keyevent(const key_event_t *ev)
{
    if (ev->type != KEY_EVENT_PRESS)
        return 0;

    uint8_t ch = ev->ascii;
    if (!ch)
        return 0;

    if (ch == '\n') {
        // line complete
        line_buffer[line_len] = '\0';
        // TODO: deliver line to shell / TTY
        line_reset();
        return 1;
    } else if (ch == '\b') {
        if (line_len > 0)
            line_len--;
    } else {
        if (line_len < LINE_BUFFER_SIZE - 1)
            line_buffer[line_len++] = ch;
    }

    return 0;
}

const char *line_get_buffer(void)
{
    return line_buffer;
}
