#include "proc/proc.h"
#include "libk/errno.h"

int proc_wait(int pid, int *status)
{
    (void)pid;
    (void)status;
    return -ENOSYS;
}