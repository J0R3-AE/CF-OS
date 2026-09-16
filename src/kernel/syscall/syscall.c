#include "syscall.h"
#include "kernel/proc/exec.h"
#include "kernel/mm/heap.h"
#include "libc/string.h"

/*
 * Kernel syscall helper prototypes.
 * These functions are kernel-side helpers and MUST NOT touch registers_t.
 * They return either an error code (<0) or a non-negative result (fd, bytes).
 */

extern int ksys_open(const char *path, int flags);
extern int ksys_close(int fd);
extern int ksys_write(int fd, const void *buf, usize count);
extern int ksys_read(int fd, void *buf, usize count);
extern int ksys_readdir(int fd, usize index, dirent_t *out);
extern int ksys_print(const char *s);
/* ksys_scan removed; use ksys_read(fd=0, ...) */

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
{
    proc_mark_exit(arg1);

    thread_t *current = sched_current();

    if (current)
        current->state = THREAD_ZOMBIE;

    sched_yield();

    regs->eax = ERR_SUCCESS;
    break;
}

    case SYS_getpid:
    {
        process_t *p = proc_current();
        regs->eax = p ? p->pid : ERR_INTERNAL;
        break;
    }

    case SYS_getppid:
    {
        process_t *p = proc_current();
        regs->eax = p ? p->ppid : ERR_INTERNAL;
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

        /* Resolve relative paths against process cwd */
        const char *use_path = path;
        char *tmp_path = NULL;
        if (path[0] != '/')
        {
            process_t *p = proc_current();
            if (!p)
            {
                regs->eax = ERR_INTERNAL;
                break;
            }

            size_t cwd_len = strlen(p->cwd);
            size_t path_len = strlen(path);
            size_t need = cwd_len + 1 + path_len + 1;
            tmp_path = malloc(need);
            if (!tmp_path)
            {
                regs->eax = ERR_INTERNAL;
                break;
            }

            if (strcmp(p->cwd, "/") == 0)
            {
                tmp_path[0] = '/';
                memcpy(tmp_path + 1, path, path_len + 1);
            }
            else
            {
                memcpy(tmp_path, p->cwd, cwd_len);
                tmp_path[cwd_len] = '/';
                memcpy(tmp_path + cwd_len + 1, path, path_len + 1);
            }

            use_path = tmp_path;
        }

        struct vnode *vn = NULL;
        int r = vfs_lookup(use_path, &vn);
        if (tmp_path)
            free(tmp_path);

        if (r < 0 || !vn)
        {
            regs->eax = ERR_NOT_FOUND;
            break;
        }

        int eret = exec_elf_vnode(vn);
        regs->eax = (eret == 0) ? ERR_SUCCESS : eret;
        break;
    }

    case SYS_chdir:
    {
        const char *path = (const char *)(uintptr_t)arg1;
        if (!path || !path[0])
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        /* Resolve to absolute path (support relative) */
        const char *use_path = path;
        char *tmp_path = NULL;
        if (path[0] != '/')
        {
            process_t *p = proc_current();
            if (!p)
            {
                regs->eax = ERR_INTERNAL;
                break;
            }

            size_t cwd_len = strlen(p->cwd);
            size_t path_len = strlen(path);
            size_t need = cwd_len + 1 + path_len + 1;
            tmp_path = malloc(need);
            if (!tmp_path)
            {
                regs->eax = ERR_INTERNAL;
                break;
            }

            if (strcmp(p->cwd, "/") == 0)
            {
                tmp_path[0] = '/';
                memcpy(tmp_path + 1, path, path_len + 1);
            }
            else
            {
                memcpy(tmp_path, p->cwd, cwd_len);
                tmp_path[cwd_len] = '/';
                memcpy(tmp_path + cwd_len + 1, path, path_len + 1);
            }

            use_path = tmp_path;
        }

        struct vnode *vn = NULL;
        int r = vfs_lookup(use_path, &vn);
        if (r != 0 || !vn)
        {
            if (tmp_path)
                free(tmp_path);
            regs->eax = ERR_NOT_FOUND;
            break;
        }

        if (vn->type != VNODE_TYPE_DIR)
        {
            if (tmp_path)
                free(tmp_path);
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        process_t *p = proc_current();
        if (!p)
        {
            if (tmp_path)
                free(tmp_path);
            regs->eax = ERR_INTERNAL;
            break;
        }

        /* copy path into process cwd */
        if (tmp_path)
        {
            strncpy(p->cwd, tmp_path, sizeof(p->cwd));
            p->cwd[sizeof(p->cwd) - 1] = '\0';
            free(tmp_path);
        }
        else
        {
            strncpy(p->cwd, use_path, sizeof(p->cwd));
            p->cwd[sizeof(p->cwd) - 1] = '\0';
        }

        regs->eax = ERR_SUCCESS;
        break;
    }

    case SYS_getcwd:
    {
        char *buf = (char *)(uintptr_t)arg1;
        int size = arg2;
        if (!buf || size <= 0)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        process_t *p = proc_current();
        if (!p)
        {
            regs->eax = ERR_INTERNAL;
            break;
        }

        int len = (int)strlen(p->cwd);
        if (len + 1 > size)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        memcpy(buf, p->cwd, (size_t)len + 1);
        regs->eax = ERR_SUCCESS;
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
        regs->eax = ksys_open(pathname, flags);
        break;
    }

    case SYS_close:
    {
        int fd = arg1;
        regs->eax = ksys_close(fd);
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

        regs->eax = ksys_write(fd, buf, (usize)count);
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

        regs->eax = ksys_read(fd, buf, (usize)count);
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
        regs->eax = ksys_readdir(fd, (usize)index, out);
        break;
    }

    /* ---------------------------------------------------------------------- */
    /* TTY Syscalls (new)                                                      */
    /* ---------------------------------------------------------------------- */
    case SYS_print:
    {
        const char *s = (const char *)(uintptr_t)arg1;
        regs->eax = ksys_print(s);
        break;
    }

    case SYS_printcolor:
    {
        regs->eax = ERR_NOT_SUPPORTED;
        break;
    }

    case SYS_scan:
    {
        regs->eax = ERR_NOT_SUPPORTED;
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