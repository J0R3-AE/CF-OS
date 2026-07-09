#include "syscall.h"
#include "libk/string.h"
#include "libk/dirent.h"

int shell(void)
{
    char buf[256];

    print("MiniOS Shell\n");

    while (1)
    {
        print("> ");

        int n = scan(buf, sizeof(buf));

        if (n <= 0)
            continue;

        /* remove trailing newline */
        if (n > 0 && buf[n - 1] == '\n')
            buf[n - 1] = '\0';

        if (buf[0] == '\0')
            continue;

        if (!strcmp(buf, "exit"))
        {
            print("bye\n");
            break;
        }

        print("You typed: ");
        print(buf);
        print("\n");
    }

    return 0;
}