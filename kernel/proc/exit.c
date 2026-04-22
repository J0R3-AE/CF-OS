#include "proc/proc.h"
#include "libk/errno.h"
#include "sched/sched.h"

int proc_exit(int code)
{
    (void)code;
    proc_mark_exit(code);
    for (;;)
        asm volatile("hlt");
}