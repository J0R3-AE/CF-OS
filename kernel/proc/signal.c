#include "proc/proc.h"
#include "libk/errno.h"

int proc_signal(int pid, int sig)
{
    (void)pid;
    (void)sig;
    return -ENOSYS;
}