#include "shared/syscall.h"

#include "libc/string.h"
#include "libc/mem.h"
#include "kernel/log.h"
#include "libc/types.h"
#include "libc/errno.h"
#include "libc/dirent.h"

#include "kernel/arch/idt.h"
#include "kernel/drivers/line.h"
#include "kernel/drivers/serial.h"
#include "kernel/drivers/tty.h"
#include "kernel/fs/fd.h"
#include "kernel/fs/vfs.h"
#include "kernel/sched/sched.h"
#include "kernel/proc/proc.h"
#include "kernel/mm/heap.h"
#include "kernel/mm/paging.h"
#include "kernel/mm/usercopy.h"

#define SYSCALL_IO_MAX (64u * 1024u)
#define SYSCALL_IO_CHUNK 256u
#define SYSCALL_PATH_MAX 256u
#define SYSCALL_PRINT_MAX 4096u

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

static bool syscall_user_range_valid(u32 address, usize length, bool write)
{
    process_t *process = proc_current();
    return process && paging_user_range_valid(process->pd, address, length,
                                               write);
}

static int syscall_resolve_path(process_t *process, const char *path,
                                char *resolved, usize capacity)
{
    usize length;
    const char *cursor;

    if (!process || !path || !path[0] || !resolved || capacity < 2)
        return ERR_INVALID_ARGUMENT;

    if (path[0] == '/')
    {
        resolved[0] = '/';
        resolved[1] = '\0';
        length = 1;
    }
    else
    {
        length = strlen(process->cwd);
        if (!length || process->cwd[0] != '/' || length >= capacity)
            return ERR_INTERNAL;
        memcpy(resolved, process->cwd, length + 1);
    }

    cursor = path;
    while (*cursor)
    {
        while (*cursor == '/')
            cursor++;
        if (!*cursor)
            break;

        const char *component = cursor;
        usize component_length = 0;
        while (cursor[component_length] && cursor[component_length] != '/')
            component_length++;
        cursor += component_length;

        if (component_length == 1 && component[0] == '.')
            continue;
        if (component_length == 2 && component[0] == '.' &&
            component[1] == '.')
        {
            if (length > 1)
            {
                while (length > 1 && resolved[length - 1] != '/')
                    length--;
                if (length > 1)
                    length--;
                resolved[length] = '\0';
            }
            continue;
        }

        usize separator = length > 1 ? 1 : 0;
        if (length >= capacity - separator ||
            component_length >= capacity - length - separator)
            return ERR_NAME_TOO_LONG;
        if (separator)
            resolved[length++] = '/';
        memcpy(resolved + length, component, component_length);
        length += component_length;
        resolved[length] = '\0';
    }

    return ERR_SUCCESS;
}

static int syscall_exec_process(u32 user_path, u32 user_argv)
{
    struct exec_request
    {
        char path[SYSCALL_PATH_MAX];
        char resolved[SYSCALL_PATH_MAX];
        char arg_storage[16][128];
        const char *argv[16];
    };

    struct exec_request *request =
        (struct exec_request *)malloc(sizeof(*request));
    if (!request)
        return ERR_OUT_OF_MEMORY;

    int result = copy_user_string(request->path, sizeof(request->path),
                                  (const char *)(uintptr_t)user_path);
    if (result != ERR_SUCCESS)
        goto done;
    if (!request->path[0])
    {
        result = ERR_INVALID_ARGUMENT;
        goto done;
    }

    int argc = 0;
    if (user_argv)
    {
        for (; argc < (int)(sizeof(request->argv) /
                            sizeof(request->argv[0])); argc++)
        {
            u32 pointer_address = user_argv + (u32)argc * sizeof(u32);
            u32 user_string;
            if (pointer_address < user_argv ||
                copy_from_user(&user_string,
                               (const void *)(uintptr_t)pointer_address,
                               sizeof(user_string)) != ERR_SUCCESS)
            {
                result = ERR_BAD_ADDRESS;
                goto done;
            }
            if (!user_string)
                break;
            result = copy_user_string(request->arg_storage[argc],
                                      sizeof(request->arg_storage[argc]),
                                      (const char *)(uintptr_t)user_string);
            if (result != ERR_SUCCESS)
                goto done;
            request->argv[argc] = request->arg_storage[argc];
        }

        if (argc == (int)(sizeof(request->argv) /
                          sizeof(request->argv[0])))
        {
            u32 terminator_address =
                user_argv + (u32)argc * sizeof(u32);
            u32 terminator;
            if (terminator_address < user_argv ||
                copy_from_user(&terminator,
                               (const void *)(uintptr_t)terminator_address,
                               sizeof(terminator)) != ERR_SUCCESS ||
                terminator != 0)
            {
                result = ERR_BUFFER_TOO_SMALL;
                goto done;
            }
        }
    }

    result = syscall_resolve_path(proc_current(), request->path,
                                  request->resolved,
                                  sizeof(request->resolved));
    if (result != ERR_SUCCESS)
        goto done;

    struct vnode *vn = NULL;
    result = vfs_lookup(request->resolved, &vn);
    if (result == ERR_SUCCESS && vn)
        result = exec_elf_vnode_args(vn, argc, request->argv);
    else if (result == ERR_SUCCESS)
        result = ERR_INTERNAL;

done:
    free(request);
    return result;
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
        regs->eax = syscall_exec_process((u32)arg1, (u32)arg2);
        break;

    case SYS_waitpid:
    {
        int child_pid = arg1;
        process_t *parent = proc_current();
        process_t *child = parent ?
            proc_find_child(parent->pid, child_pid) : NULL;
        if (arg3 != 0)
        {
            regs->eax = ERR_NOT_SUPPORTED;
            break;
        }
        if (!child)
        {
            regs->eax = ERR_NO_CHILD_PROCESS;
            break;
        }

        while (child->alive)
            ksched_yield();

        int status = child->exit_code;
        if (arg2)
        {
            int copy_result = copy_to_user((void *)(uintptr_t)arg2, &status,
                                           sizeof(status));
            if (copy_result != ERR_SUCCESS)
            {
                regs->eax = copy_result;
                break;
            }
        }

        if (child_pid > 0 && child_pid < 64 &&
            g_fd_tables_used[child_pid])
        {
            fd_table_t *child_fds = &g_fd_tables[child_pid];
            for (int fd = 0; fd < FD_MAX; fd++)
            {
                if (child_fds->entries[fd].used)
                    fd_close(child_fds, fd);
            }
            g_fd_tables_used[child_pid] = 0;
        }
        int reap_result = proc_reap(child_pid);
        regs->eax = reap_result == ERR_SUCCESS ? child_pid : reap_result;
        break;
    }

    /* ---------------------------------------------------------------------- */
    /* File I/O                                                                */
    /* ---------------------------------------------------------------------- */
    case SYS_open:
    {
        char pathname[SYSCALL_PATH_MAX];
        char resolved[SYSCALL_PATH_MAX];
        int flags = arg2;
        int copy_result = copy_user_string(pathname, sizeof(pathname),
                                           (const char *)(uintptr_t)arg1);
        if (copy_result != ERR_SUCCESS)
        {
            regs->eax = copy_result;
            break;
        }

        copy_result = syscall_resolve_path(proc_current(), pathname, resolved,
                                           sizeof(resolved));
        if (copy_result != ERR_SUCCESS)
        {
            regs->eax = copy_result;
            break;
        }

        struct file *f = NULL;
        int r = vfs_open(resolved, (u32)flags, &f);
        if (r != ERR_SUCCESS || !f)
        {
            regs->eax = (r == ERR_SUCCESS) ? ERR_INTERNAL : r;
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
            regs->eax = fd;
            break;
        }

        file_unref(f);
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
        const void *user_buf = (const void *)(uintptr_t)arg2;
        int count = arg3;

        if (!user_buf || count <= 0)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }
        if ((u32)count > SYSCALL_IO_MAX)
        {
            regs->eax = ERR_BUFFER_TOO_SMALL;
            break;
        }
        if (!syscall_user_range_valid((u32)arg2, (usize)count, false))
        {
            regs->eax = ERR_BAD_ADDRESS;
            break;
        }

        struct file *f = NULL;
        if (fd != 1 && fd != 2)
        {
            fd_table_t *fdt = syscall_get_fd_table();
            f = fdt ? fd_get(fdt, fd) : NULL;
            if (!f)
            {
                regs->eax = ERR_BAD_FILE_DESCRIPTOR;
                break;
            }
        }

        char chunk[SYSCALL_IO_CHUNK];
        usize total = 0;
        while (total < (usize)count)
        {
            usize length = (usize)count - total;
            if (length > sizeof(chunk))
                length = sizeof(chunk);

            int r = copy_from_user(chunk,
                                   (const u8 *)user_buf + total, length);
            if (r != ERR_SUCCESS)
            {
                regs->eax = total ? (int)total : r;
                break;
            }

            if (fd == 1 || fd == 2)
            {
                for (usize i = 0; i < length; i++)
                {
                    TTY_putc(chunk[i]);
                    serial_write_char(chunk[i]);
                }
                total += length;
                continue;
            }

            usize written = 0;
            r = vfs_write(f, chunk, length, &written);
            if (written > length)
            {
                regs->eax = total ? (int)total : ERR_INTERNAL;
                break;
            }
            total += written;
            if (r != ERR_SUCCESS || written < length)
            {
                regs->eax = total ? (int)total : r;
                break;
            }
        }

        if (total == (usize)count)
            regs->eax = (int)total;
        break;
    }

    case SYS_read:
    {
        int fd = arg1;
        void *user_buf = (void *)(uintptr_t)arg2;
        int count = arg3;

        if (!user_buf || count <= 0)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }
        if ((u32)count > SYSCALL_IO_MAX)
        {
            regs->eax = ERR_BUFFER_TOO_SMALL;
            break;
        }
        if (!syscall_user_range_valid((u32)arg2, (usize)count, true))
        {
            regs->eax = ERR_BAD_ADDRESS;
            break;
        }

        char chunk[SYSCALL_IO_CHUNK];
        if (fd == 0)
        {
            usize length = (usize)count;
            if (length > sizeof(chunk))
                length = sizeof(chunk);
            int r = line_read(chunk, length, true);
            if (r < 0)
            {
                regs->eax = r;
                break;
            }
            usize nread = (usize)r;
            r = copy_to_user(user_buf, chunk, nread);
            regs->eax = r == ERR_SUCCESS ? (int)nread : r;
            break;
        }

        fd_table_t *fdt = syscall_get_fd_table();
        struct file *f = fdt ? fd_get(fdt, fd) : NULL;

        if (!f)
        {
            regs->eax = ERR_BAD_FILE_DESCRIPTOR;
            break;
        }

        usize total = 0;
        while (total < (usize)count)
        {
            usize length = (usize)count - total;
            if (length > sizeof(chunk))
                length = sizeof(chunk);

            usize nread = 0;
            int r = vfs_read(f, chunk, length, &nread);
            if (r != ERR_SUCCESS)
            {
                regs->eax = total ? (int)total : r;
                break;
            }
            if (nread > length)
            {
                regs->eax = total ? (int)total : ERR_INTERNAL;
                break;
            }
            if (!nread)
                break;

            r = copy_to_user((u8 *)user_buf + total, chunk, nread);
            if (r != ERR_SUCCESS)
            {
                regs->eax = total ? (int)total : r;
                break;
            }

            total += nread;
            if (nread < length)
                break;
        }

        if (total == (usize)count || total)
            regs->eax = (int)total;
        else
            regs->eax = ERR_SUCCESS;
        break;
    }

    /* ---------------------------------------------------------------------- */
    /* Directory / FS                                                          */
    /* ---------------------------------------------------------------------- */
    case SYS_readdir:
    {
        int fd = arg1;
        int index = arg2;

        if (!syscall_user_range_valid((u32)arg3, sizeof(dirent_t), true))
        {
            regs->eax = ERR_BAD_ADDRESS;
            break;
        }

        fd_table_t *fdt = syscall_get_fd_table();
        struct file *f = fdt ? fd_get(fdt, fd) : NULL;

        if (!f || !f->vn || !f->vn->ops || !f->vn->ops->readdir)
        {
            regs->eax = ERR_BAD_FILE_DESCRIPTOR;
            break;
        }
        if (index < 0)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        const char *name = NULL;
        vnode_type_t type;

        int r = f->vn->ops->readdir(f->vn, (usize)index, &name, &type);
        if (r != ERR_SUCCESS)
        {
            regs->eax = r;
            break;
        }
        if (!name)
        {
            regs->eax = ERR_INTERNAL;
            break;
        }

        dirent_t entry = {0};
        usize name_length = strlen(name);
        if (name_length >= sizeof(entry.name))
            name_length = sizeof(entry.name) - 1;
        memcpy(entry.name, name, name_length);
        entry.name[name_length] = '\0';
        entry.type = (unsigned char)type;
        entry.length = (unsigned short)name_length;

        int copy_result = copy_to_user((void *)(uintptr_t)arg3, &entry,
                                       sizeof(entry));
        if (copy_result != ERR_SUCCESS)
        {
            regs->eax = copy_result;
            break;
        }

        regs->eax = ERR_SUCCESS;
        break;
    }

    case SYS_lseek:
    {
        int fd = arg1;
        int offset = arg2;
        int whence = arg3;
        fd_table_t *fdt = syscall_get_fd_table();
        struct file *f = fdt ? fd_get(fdt, fd) : NULL;

        if (!f)
            regs->eax = ERR_BAD_FILE_DESCRIPTOR;
        else
        {
            s64 base;
            if (whence == MINIOS_SEEK_SET)
                base = 0;
            else if (whence == MINIOS_SEEK_CUR)
                base = (s64)f->offset;
            else if (whence == MINIOS_SEEK_END)
            {
                vfs_stat_t stat;
                if (!f->vn || !f->vn->ops || !f->vn->ops->getattr)
                {
                    regs->eax = ERR_NOT_SUPPORTED;
                    break;
                }
                int stat_result = f->vn->ops->getattr(f->vn, &stat);
                if (stat_result != ERR_SUCCESS)
                {
                    regs->eax = stat_result;
                    break;
                }
                base = (s64)stat.size;
            }
            else
            {
                regs->eax = ERR_INVALID_ARGUMENT;
                break;
            }

            s64 target = base + (s64)offset;
            if (target < 0 || target > 0x7FFFFFFF)
                regs->eax = ERR_INVALID_ARGUMENT;
            else
            {
                int result = vfs_seek(f, (usize)target);
                regs->eax = result == ERR_SUCCESS ? (int)target : result;
            }
        }
        break;
    }

    case SYS_mkdir:
    case SYS_rmdir:
    case SYS_unlink:
    {
        char pathname[SYSCALL_PATH_MAX];
        char resolved[SYSCALL_PATH_MAX];
        int result = copy_user_string(pathname, sizeof(pathname),
                                      (const char *)(uintptr_t)arg1);
        if (result != ERR_SUCCESS)
        {
            regs->eax = result;
            break;
        }

        result = syscall_resolve_path(proc_current(), pathname, resolved,
                                      sizeof(resolved));
        if (result != ERR_SUCCESS)
        {
            regs->eax = result;
            break;
        }

        if (num == SYS_mkdir)
        {
            regs->eax = vfs_mkdir(resolved);
            break;
        }

        struct vnode *target = NULL;
        result = vfs_lookup(resolved, &target);
        if (result != ERR_SUCCESS)
        {
            regs->eax = result;
            break;
        }
        if (num == SYS_rmdir && target->type != VNODE_TYPE_DIR)
        {
            regs->eax = ERR_NOT_DIRECTORY;
            break;
        }
        if (num == SYS_unlink && target->type == VNODE_TYPE_DIR)
        {
            regs->eax = ERR_IS_DIRECTORY;
            break;
        }

        regs->eax = vfs_unlink(resolved);
        break;
    }

    case SYS_stat:
    {
        char pathname[SYSCALL_PATH_MAX];
        char resolved[SYSCALL_PATH_MAX];
        vfs_stat_t stat;
        int result = copy_user_string(pathname, sizeof(pathname),
                                      (const char *)(uintptr_t)arg1);
        if (result != ERR_SUCCESS)
        {
            regs->eax = result;
            break;
        }
        result = syscall_resolve_path(proc_current(), pathname, resolved,
                                      sizeof(resolved));
        if (result != ERR_SUCCESS)
        {
            regs->eax = result;
            break;
        }
        if (!syscall_user_range_valid((u32)arg2, sizeof(stat), true))
        {
            regs->eax = ERR_BAD_ADDRESS;
            break;
        }

        result = vfs_stat(resolved, &stat);
        if (result == ERR_SUCCESS)
            result = copy_to_user((void *)(uintptr_t)arg2, &stat, sizeof(stat));
        regs->eax = result;
        break;
    }

    case SYS_chdir:
    {
        char pathname[SYSCALL_PATH_MAX];
        char resolved[SYSCALL_PATH_MAX];
        int result = copy_user_string(pathname, sizeof(pathname),
                                      (const char *)(uintptr_t)arg1);
        if (result == ERR_SUCCESS)
            result = syscall_resolve_path(proc_current(), pathname, resolved,
                                          sizeof(resolved));
        if (result == ERR_SUCCESS)
        {
            struct vnode *directory = NULL;
            result = vfs_lookup(resolved, &directory);
            if (result == ERR_SUCCESS && directory->type != VNODE_TYPE_DIR)
                result = ERR_NOT_DIRECTORY;
            if (result == ERR_SUCCESS)
            {
                process_t *process = proc_current();
                usize length = strlen(resolved);
                if (length >= sizeof(process->cwd))
                    result = ERR_NAME_TOO_LONG;
                else
                    memcpy(process->cwd, resolved, length + 1);
            }
        }
        regs->eax = result;
        break;
    }

    case SYS_getcwd:
    {
        process_t *process = proc_current();
        usize length;
        if (!process || arg2 <= 0 ||
            !syscall_user_range_valid((u32)arg1, (usize)arg2, true))
        {
            regs->eax = ERR_BAD_ADDRESS;
            break;
        }
        length = strlen(process->cwd);
        if (length + 1 > (usize)arg2)
            regs->eax = ERR_BUFFER_TOO_SMALL;
        else
        {
            int result = copy_to_user((void *)(uintptr_t)arg1, process->cwd,
                                      length + 1);
            regs->eax = result == ERR_SUCCESS ? (int)length : result;
        }
        break;
    }

    case SYS_ftruncate:
    {
        int fd = arg1;
        int length = arg2;
        fd_table_t *fdt = syscall_get_fd_table();
        struct file *f = fdt ? fd_get(fdt, fd) : NULL;

        if (!f)
            regs->eax = ERR_BAD_FILE_DESCRIPTOR;
        else if (length < 0)
            regs->eax = ERR_INVALID_ARGUMENT;
        else if (!f->vn || !f->vn->ops || !f->vn->ops->truncate)
            regs->eax = ERR_NOT_SUPPORTED;
        else
            regs->eax = f->vn->ops->truncate(f->vn, (usize)length);
        break;
    }

    /* ---------------------------------------------------------------------- */
    /* TTY Syscalls (new)                                                      */
    /* ---------------------------------------------------------------------- */
    case SYS_print:
    {
        char *text = malloc(SYSCALL_PRINT_MAX);
        if (!text)
        {
            regs->eax = ERR_OUT_OF_MEMORY;
            break;
        }

        int copy_result = copy_user_string(text, SYSCALL_PRINT_MAX,
                                           (const char *)(uintptr_t)arg1);
        if (copy_result != ERR_SUCCESS)
        {
            free(text);
            regs->eax = copy_result;
            break;
        }

        TTY_puts(text);
        free(text);
        regs->eax = ERR_SUCCESS;
        break;
    }

    case SYS_clear:
        TTY_clear();
        regs->eax = ERR_SUCCESS;
        break;

    case SYS_printcolor:
    {
        if (arg2 < 0 || arg2 > 15 || arg3 < 0 || arg3 > 15)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }

        char *text = malloc(SYSCALL_PRINT_MAX);
        if (!text)
        {
            regs->eax = ERR_OUT_OF_MEMORY;
            break;
        }

        int copy_result = copy_user_string(text, SYSCALL_PRINT_MAX,
                                           (const char *)(uintptr_t)arg1);
        if (copy_result != ERR_SUCCESS)
        {
            free(text);
            regs->eax = copy_result;
            break;
        }

        u8 old_foreground;
        u8 old_background;
        TTY_getcolor(&old_foreground, &old_background);
        TTY_setcolor((u8)arg2, (u8)arg3);
        TTY_puts(text);
        TTY_setcolor(old_foreground, old_background);
        free(text);
        regs->eax = ERR_SUCCESS;
        break;
    }

    case SYS_setcolor:
        if (arg1 < 0 || arg1 > 15 || arg2 < 0 || arg2 > 15)
            regs->eax = ERR_INVALID_ARGUMENT;
        else
        {
            TTY_setcolor((u8)arg1, (u8)arg2);
            regs->eax = ERR_SUCCESS;
        }
        break;

    case SYS_scan:
    {
        void *user_buf = (void *)(uintptr_t)arg1;
        int count = arg2;

        if (!user_buf || count <= 0)
        {
            regs->eax = ERR_INVALID_ARGUMENT;
            break;
        }
        if ((u32)count > SYSCALL_IO_MAX)
        {
            regs->eax = ERR_BUFFER_TOO_SMALL;
            break;
        }
        if (!syscall_user_range_valid((u32)arg1, (usize)count, true))
        {
            regs->eax = ERR_BAD_ADDRESS;
            break;
        }

        char *buffer = malloc((usize)count);
        if (!buffer)
        {
            regs->eax = ERR_OUT_OF_MEMORY;
            break;
        }

        memset(buffer, 0, (usize)count);
        int result = line_scan(buffer, (usize)count);
        usize output_length = strlen(buffer) + 1;
        int copy_result = copy_to_user(user_buf, buffer, output_length);
        free(buffer);

        regs->eax = copy_result == ERR_SUCCESS ? result : copy_result;
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
