#include "user/commands.h"

#include "shared/syscall.h"

int cmd_ver(int argc, char **argv)
{
    (void)argv;
    if (argc != 0)
    {
        print("Usage: ver\n");
        return -1;
    }

    return print("AescOS version 0.1\n");
}
