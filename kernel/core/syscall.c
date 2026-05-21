#include "syscall.h"
#include "proc/proc.h"
#include "fs/vfs.h"
#include "fs/fd.h"
#include "arch/idt.h"
#include "arch/io.h"
#include "sched/sched.h"
#include "drivers/tty.h"
#include "drivers/kbd.h"
#include "drivers/keyboard.h"
#include "libk/log.h"
#include "libk/errno.h"

// Global to track file descriptor tables per process
// TODO: move this to process_t structure
static fd_table_t g_fd_tables[64];
static int g_fd_tables_used[64];

fd_table_t *syscall_get_fd_table(void)
{
    process_t *p = proc_current();
    if (!p || p->pid >= 64)
        return NULL;
    
    if (!g_fd_tables_used[p->pid])
    {
        fd_table_init(&g_fd_tables[p->pid]);
        g_fd_tables_used[p->pid] = 1;
    }
    
    return &g_fd_tables[p->pid];
}

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
        {
            int fd = arg1;
            const char *buf = (const char *)(uintptr_t)arg2;
            int count = arg3;
            
            if (count <= 0 || !buf)
            {
                regs->eax = -1;
                break;
            }

            if (fd == 1 || fd == 2)  // stdout or stderr
            {
                int written = 0;
                for (int i = 0; i < count; i++)
                {
                    TTY_putc(buf[i]);
                    i386SERIAL_write(buf[i]);
                    written++;
                }
                regs->eax = written;
            }
            else
            {
                fd_table_t *fdt = syscall_get_fd_table();
                struct file *f = fdt ? fd_get(fdt, fd) : NULL;
                if (f)
                {
                    usize written = 0;
                    int ret = vfs_write(f, (void *)buf, count, &written);
                    regs->eax = (ret == 0) ? written : -1;
                }
                else
                {
                    regs->eax = -1;  // EBADF
                }
            }
            break;
        }

        case SYS_read:
        {
            int fd = arg1;
            char *buf = (char *)(uintptr_t)arg2;
            int count = arg3;
            
            if (count <= 0 || !buf)
            {
                regs->eax = -1;
                break;
            }

            if (fd == 0)  // stdin - keyboard input
            {
                // Read individual characters with echo
                int nread = 0;
                while (nread < count)
                {
                    int key = kbd_try_getchar();
                    
                    if (key < 0)
                    {
                        // No input available - yield and try again
                        ksched_yield();
                        continue;
                    }

                    // Echo the character
                    TTY_putc((char)key);
                    
                    // Store in buffer
                    buf[nread++] = (char)key;
                    
                    // Stop at newline
                    if (key == '\n' || key == KEY_ENTER)
                        break;
                }
                regs->eax = nread;
            }
            else
            {
                fd_table_t *fdt = syscall_get_fd_table();
                struct file *f = fdt ? fd_get(fdt, fd) : NULL;
                if (f)
                {
                    usize nread = 0;
                    int ret = vfs_read(f, buf, count, &nread);
                    regs->eax = (ret == 0) ? (int)nread : -1;
                }
                else
                {
                    regs->eax = -1;  // EBADF
                }
            }
            break;
        }

        case SYS_open:
        {
            const char *pathname = (const char *)(uintptr_t)arg1;
            int flags = arg2;
            
            struct file *f = NULL;
            int ret = vfs_open(pathname, flags, &f);
            if (ret != 0 || !f)
            {
                regs->eax = -1;  // ENOENT
                break;
            }

            fd_table_t *fdt = syscall_get_fd_table();
            if (!fdt)
            {
                regs->eax = -1;
                break;
            }

            int fd = fd_alloc(fdt, f);
            regs->eax = fd;
            break;
        }

        case SYS_close:
        {
            int fd = arg1;
            fd_table_t *fdt = syscall_get_fd_table();
            if (!fdt)
            {
                regs->eax = -1;
                break;
            }

            int ret = fd_close(fdt, fd);
            regs->eax = ret;
            break;
        }

        case SYS_getpid:
        {
            process_t *p = proc_current();
            regs->eax = p ? p->pid : -1;
            break;
        }

        case SYS_fork:
        {
            // TODO: full fork implementation with page directory copy
            KLOG_WARN("SYS_fork not yet implemented");
            regs->eax = -1;
            break;
        }

        case SYS_execve:
        {
            // TODO: execve implementation
            KLOG_WARN("SYS_execve not yet implemented");
            regs->eax = -1;
            break;
        }

        case SYS_waitpid:
        {
            // TODO: waitpid implementation
            KLOG_WARN("SYS_waitpid not yet implemented");
            regs->eax = -1;
            break;
        }

        default:
            KLOG_WARN("Unknown syscall: %d", syscall_num);
            regs->eax = -1;
            break;
    }
}