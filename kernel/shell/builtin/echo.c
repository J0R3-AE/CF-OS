#include "builtin.h"
#include "drivers/fbcon.h"

int builtin_echo(const char *args, char **argv)
{
    while (*args == ' ') args++;
    printf("%s\n", args);
}
