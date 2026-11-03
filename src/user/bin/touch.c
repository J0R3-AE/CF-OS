#include "user/commands.h"

#include "shared/syscall.h"

int cmd_touch(int argc, char **argv)
{
    if (argc != 1 || !argv || !argv[0])
    {
        print("Usage: touch <file>\n");
        return -1;
    }

    int fd = open(argv[0], MINIOS_O_RDWR | MINIOS_O_CREATE);
    return fd < 0 ? fd : close(fd);
}
