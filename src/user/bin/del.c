#include "user/commands.h"

#include "shared/syscall.h"

int cmd_del(int argc, char **argv)
{
    if (argc != 1 || !argv || !argv[0])
    {
        print("Usage: del <file>\n");
        return -1;
    }
    return unlink(argv[0]);
}
