#include "libc/errno.h"

#ifndef APPLET_ENTRY
#error APPLET_ENTRY must name a command function
#endif

extern int APPLET_ENTRY(int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc < 1 || !argv)
        return ERR_INVALID_ARGUMENT;
    return APPLET_ENTRY(argc - 1, argv + 1);
}
