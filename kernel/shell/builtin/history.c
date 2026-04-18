#include "builtin.h"
#include "drivers/fbcon.h"

#define HIST_MAX 64
static char history[HIST_MAX][128];
static usize hist_count = 0;

void shell_history_add(const char *line)
{
    strncpy(history[hist_count % HIST_MAX], line, 127);
    history[hist_count % HIST_MAX][127] = '\0';
    hist_count++;
}

usize shell_history_count(void)
{
    return (hist_count < HIST_MAX) ? hist_count : HIST_MAX;
}

const char *shell_history_get(usize index)
{
    if (index >= shell_history_count())
        return NULL;

    usize base = (hist_count >= HIST_MAX) ? (hist_count - HIST_MAX) : 0;
    return history[base + index];
}

int builtin_history(int argc, char **argv)
{
    (void)argc; (void)argv;

    size_t count = shell_history_count();
    for (size_t i = 0; i < count; ++i) {
        printf("  ");
        // simple index print; replace with real printf if you have it
        // or skip index entirely:
        // fbcon_puts(shell_history_get(i));
        printf(shell_history_get(i));
        printf('\n');
    }

    return 0;
}
