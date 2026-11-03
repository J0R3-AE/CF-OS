#include "user/commands.h"

#include "shared/syscall.h"

int cmd_mkdir(int argc, char **argv)
{
    if (argc != 1 || !argv || !argv[0])
    {
        print("Usage: mkdir <directory>\n");
        return -1;
    }
    return mkdir(argv[0]);
}
