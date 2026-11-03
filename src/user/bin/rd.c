#include "user/commands.h"

#include "shared/syscall.h"

int cmd_rd(int argc, char **argv)
{
    if (argc != 1 || !argv || !argv[0])
    {
        print("Usage: rd <directory>\n");
        return -1;
    }
    return rmdir(argv[0]);
}
