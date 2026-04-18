#include "builtin.h"
#include "drivers/fbcon.h"

extern char cwd[128];

int builtin_pwd(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf(cwd);
    printf('\n');
    return 0;
}
