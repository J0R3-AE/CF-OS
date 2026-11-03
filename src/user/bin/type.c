#include "user/commands.h"

#include "shared/syscall.h"

int cmd_type(int argc, char **argv)
{
    char buffer[256];
    int fd;

    if (argc != 1 || !argv || !argv[0])
    {
        print("Usage: type <file>\n");
        return -1;
    }

    fd = open(argv[0], MINIOS_O_RDONLY);
    if (fd < 0)
        return fd;

    int result = 0;
    for (;;)
    {
        int count = read(fd, buffer, sizeof(buffer));
        if (count < 0)
        {
            result = count;
            break;
        }
        if (count == 0)
            break;

        int written = 0;
        while (written < count)
        {
            int current = write(1, buffer + written, count - written);
            if (current <= 0)
            {
                result = current < 0 ? current : -1;
                break;
            }
            written += current;
        }
        if (result != 0)
            break;
    }

    int close_result = close(fd);
    return result != 0 ? result : close_result;
}
