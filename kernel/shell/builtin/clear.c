#include "builtin.h"
#include "drivers/fbcon.h"

int builtin_clear(int argc, char **argv)
{
    (void)argc; (void)argv;
    fbcon_clear();
    return 0;
}
