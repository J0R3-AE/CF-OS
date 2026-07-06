
#include "syscall.h"
#include "libk/dirent.h"

int ls(int argc, char **argv)
{
    {
    int fd = open(".", 0);
    if (fd < 0)
        return 1;

    dirent_t ent;
    int index = 0;

    while (readdir(fd, index, &ent) == 0)
    {
        print(ent.name);
        print("\n");
        index++;
    }

    close(fd);
    return 0;
}
}