#include "user/commands.h"

#include "shared/syscall.h"

#define MORE_PAGE_LINES 20
#define MORE_BUFFER_SIZE 256

int cmd_more(int argc, char **argv)
{
    char buffer[MORE_BUFFER_SIZE];
    char response[16];
    int lines = 0;

    if (argc != 1 || !argv || !argv[0])
    {
        print("Usage: more <file>\n");
        return -1;
    }

    int fd = open(argv[0], MINIOS_O_RDONLY);
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

        for (int i = 0; i < count; i++)
        {
            char character[2] = {buffer[i], '\0'};
            print(character);
            if (buffer[i] != '\n')
                continue;

            if (++lines == MORE_PAGE_LINES)
            {
                print("-- More (Enter to continue, q to quit) -- ");
                int input = scan(response, sizeof(response));
                if (input < 0)
                {
                    result = input;
                    break;
                }
                if (response[0] == 'q' || response[0] == 'Q')
                    goto done;
                lines = 0;
            }
        }
        if (result != 0)
            break;
    }

done:
    {
        int close_result = close(fd);
        return result != 0 ? result : close_result;
    }
}
