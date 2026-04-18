#include "builtin.h"
#include "drivers/fbcon.h"

int builtin_help(int argc, char **argv)
{
    (void)argc; (void)argv;

    printf("Built-in commands:\n");

    for (size_t i = 0;; ++i) {
        const builtin_cmd_t *b = builtin_iter(i);
        if (!b)
            break;

        printf("  ");
        printf(b->name);
        if (b->help) {
            printf(" - ");
            printf(b->help);
        }
        printf('\n');
    }

    return 0;
}
