#include "syscall.h"
#include "proc/proc.h"
#include "fs/vfs.h"
#include "fs/fd.h"
#include "arch/idt.h"
#include "arch/io.h"
#include "sched/sched.h"
#include "drivers/tty.h"
#include "libk/log.h"

// Syscall handler
void syscall_handler(registers_t *regs) {
    int syscall_num = regs->eax;
    int arg1 = regs->ebx;
    int arg2 = regs->ecx;
    int arg3 = regs->edx;

    switch (syscall_num) {
        case SYS_exit:
            proc_mark_exit(arg1);
            if (g_current)
                g_current->state = THREAD_ZOMBIE;
            ksched_yield();
            regs->eax = 0;
            break;
        case SYS_write:
            if (arg1 == 1 && arg2 && arg3 > 0)
            {
                const char *buf = (const char *)(uintptr_t)arg2;
                KLOG_INFO("syscall: SYS_write fd=1 len=%d buf=%p", arg3, buf);
                int written = 0;
                for (int i = 0; i < arg3; i++)
                {
                    TTY_putc(buf[i]);
                    i386SERIAL_write(buf[i]);
                    written++;
                }
                regs->eax = written;
            }
            else
            {
                regs->eax = -1;
            }
            break;
        case SYS_read:
            regs->eax = 0;
            break;
        default:
            regs->eax = -1;
            break;
    }
}