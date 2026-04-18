#include "builtin.h"
#include "drivers/fbcon.h"

int builtin_exit(int argc, char **argv)
{
    (void)argc; (void)argv;
    //shell_request_exit();
    return 1;
}
