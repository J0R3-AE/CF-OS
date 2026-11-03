
#include "user/commands.h"

#include "shared/syscall.h"
#include "libc/dirent.h"
#include "libc/errno.h"

int cmd_dir(int argc, char **argv)
{
    const char *path = argc > 0 && argv && argv[0] ? argv[0] : ".";

    int fd = open(path, MINIOS_O_RDONLY);
    if (fd < 0)
        return fd;

    dirent_t ent;
    int index = 0;

    int result = 0;
    for (;;)
    {
        int read_result = readdir(fd, index, &ent);
        if (read_result == ERR_NOT_FOUND)
            break;
        if (read_result < 0)
        {
            result = read_result;
            break;
        }
        print(ent.name);
        print("\n");
        index++;
    }

    int close_result = close(fd);
    return result != 0 ? result : close_result;
}