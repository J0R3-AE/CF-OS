#include "syscall.h"

#include "libc/string.h"
#include "libc/mem.h"
#include "libc/log.h"
#include "libc/types.h"
#include "libc/errno.h"
#include "libc/dirent.h"

#include "kernel/arch/idt.h"
#include "kernel/arch/io.h"

#include "kernel/fs/fd.h"
#include "kernel/fs/vfs.h"

#include "kernel/drivers/tty.h"
#include "kernel/drivers/line.h"

#include "kernel/sched/sched.h"
#include "kernel/proc/proc.h"


/* TODO: move per‑process fd tables into process_t */
static fd_table_t g_fd_tables[64];
static int g_fd_tables_used[64];

static fd_table_t *syscall_get_fd_table(void)
{
    process_t *p = proc_current();
    if (!p || p->pid < 0 || p->pid >= 64)
        return NULL;

    if (!g_fd_tables_used[p->pid])
    {
        fd_table_init(&g_fd_tables[p->pid]);
        g_fd_tables_used[p->pid] = 1;
    }

    return &g_fd_tables[p->pid];
}

void syscall_handler(registers_t *regs)
{
    int num = regs->eax;
    int arg1 = regs->ebx;
    int arg2 = regs->ecx;
    int arg3 = regs->edx;

    switch (num)
    {
    /* ---------------------------------------------------------------------- */
    /* Process Control                                                         */
    /* ---------------------------------------------------------------------- */
    case SYS_exit:
        proc_mark_exit(arg1);
        if (g_current)
            g_current->state = THREAD_ZOMBIE;
        ksched_yield();
        regs->eax = ERR_SUCCESS;
        break;

    case SYS_getpid:
    {
        process_t *p = proc_current();
        regs->eax = p ? p->pid : ERR_INTERNAL;
        break;
    }

    case SYS_fork:
        regs->eax = ERR_NOT_SUPPORTED;
        break;

    case SYS_execve:
    {
        const char *path = (const char *)(uintptr_t)arg1;
        char *const *argv = (char *const *)(uintptr_t)arg2;

        (void)argv;

        if (!path || !path[0])
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        struct vnode *vn = NULL;
        int r = vfs_lookup(path, &vn);
        if (r < 0 || !vn)
        {
            regs->eax = ERR_NOT_FOUND;
            break;
        }

        int eret = exec_elf_vnode(vn);
        regs->eax = (eret == 0) ? ERR_SUCCESS : eret;
        break;
    }

    case SYS_waitpid:
        regs->eax = ERR_NOT_SUPPORTED;
        break;

    /* ---------------------------------------------------------------------- */
    /* File I/O                                                                */
    /* ---------------------------------------------------------------------- */
    case SYS_open:
    {
        const char *pathname = (const char *)(uintptr_t)arg1;
        int flags = arg2;

        struct file *f = NULL;
        int r = vfs_open(pathname, (u32)flags, &f);
        if (r < 0 || !f)
        {
            regs->eax = ERR_NOT_FOUND;
            break;
        }

        fd_table_t *fdt = syscall_get_fd_table();
        if (!fdt)
        {
            file_unref(f);
            regs->eax = ERR_INTERNAL;
            break;
        }

        int fd = fd_alloc(fdt, f);
        if (fd < 0)
        {
            file_unref(f);
            regs->eax = ERR_TOO_MANY_OPEN_FILES;
            break;
        }

        regs->eax = fd;
        break;
    }

    case SYS_close:
    {
        int fd = arg1;
        fd_table_t *fdt = syscall_get_fd_table();
        if (!fdt)
        {
            regs->eax = ERR_INTERNAL;
            break;
        }

        int r = fd_close(fdt, fd);
        regs->eax = (r == 0) ? ERR_SUCCESS : ERR_BAD_FILE_DESCRIPTOR;
        break;
    }

    case SYS_write:
    {
        int fd = arg1;
        const char *buf = (const char *)(uintptr_t)arg2;
        int count = arg3;

        if (!buf || count <= 0)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        // Print fallback
        if (fd == 1 || fd == 2) // stdout or stderr
        {
            int written = 0;
            for (int i = 0; i < count; i++)
            {
                TTY_putc(buf[i]);
                i386SERIAL_write(buf[i]);
                written++;
            }
            regs->eax = written;
            break;
        }

        fd_table_t *fdt = syscall_get_fd_table();
        struct file *f = fdt ? fd_get(fdt, fd) : NULL;

        if (!f)
        {
            regs->eax = ERR_BAD_FILE_DESCRIPTOR;
            break;
        }

        usize written = 0;
        int r = vfs_write(f, buf, (usize)count, &written);
        regs->eax = (r == 0) ? (int)written : r;
        break;
    }

    case SYS_read:
    {
        int fd = arg1;
        char *buf = (char *)(uintptr_t)arg2;
        int count = arg3;

        if (!buf || count <= 0)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        /* stdin: cooked, non-blocking line read (partial line OK) */
        if (fd == 0)
        {
            //regs->eax = line_get_buffer(buf, count);
            break;
        }

        fd_table_t *fdt = syscall_get_fd_table();
        struct file *f = fdt ? fd_get(fdt, fd) : NULL;

        if (!f)
        {
            regs->eax = ERR_BAD_FILE_DESCRIPTOR;
            break;
        }

        usize nread = 0;
        int r = vfs_read(f, buf, (usize)count, &nread);
        regs->eax = (r == 0) ? (int)nread : r;
        break;
    }

    /* ---------------------------------------------------------------------- */
    /* Directory / FS                                                          */
    /* ---------------------------------------------------------------------- */
    case SYS_readdir:
    {
        int fd = arg1;
        int index = arg2;
        dirent_t *out = (dirent_t *)(uintptr_t)arg3;

        fd_table_t *fdt = syscall_get_fd_table();
        struct file *f = fdt ? fd_get(fdt, fd) : NULL;

        if (!f || !out || !f->vn || !f->vn->ops || !f->vn->ops->readdir)
        {
            regs->eax = ERR_BAD_FILE_DESCRIPTOR;
            break;
        }

        const char *name = NULL;
        vnode_type_t type;

        int r = f->vn->ops->readdir(f->vn, (usize)index, &name, &type);
        if (r < 0 || !name)
        {
            regs->eax = ERR_NOT_FOUND;
            break;
        }

        strncpy(out->name, name, sizeof(out->name));
        out->name[sizeof(out->name) - 1] = '\0';
        out->type = type;
        out->length = strlen(name);

        regs->eax = ERR_SUCCESS;
        break;
    }

    /* ---------------------------------------------------------------------- */
    /* TTY Syscalls (new)                                                      */
    /* ---------------------------------------------------------------------- */
    case SYS_print:
    {
        const char *s = (const char *)(uintptr_t)arg1;

        if (!s)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        while (*s)
        {
            TTY_putc(*s);
            i386SERIAL_write(*s);
            s++;
        }

        regs->eax = ERR_SUCCESS;
        break;
    }

    case SYS_printcolor:
    {
        regs->eax = ERR_NOT_SUPPORTED;
        break;
    }

    case SYS_scan:
    {
        char *buf = (char *)(uintptr_t)arg1;
        int count = arg2;

        if (!buf || count <= 0)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        /* blocking cooked line read: waits for ENTER, echoes, handles backspace */
        //size_t n = line_get_buffer();(buf, (size_t)count);

        size_t n = 0;
        regs->eax = (int)n;
        break;
    }

    /* ---------------------------------------------------------------------- */
    /* Unknown                                                                 */
    /* ---------------------------------------------------------------------- */
    default:
        KLOG_WARN("Unknown syscall: %d", num);
        regs->eax = ERR_NOT_SUPPORTED;
        break;
    }
}
