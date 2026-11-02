#include "kernel/fs/fd.h"
#include "kernel/fs/vfs.h"    /* for vfs_close / file struct */
#include "libc/mem.h"
#include "libc/string.h"
#include "libc/errno.h"

void fd_table_init(fd_table_t *t)
{
    if (!t)
        return;
    memset(t, 0, sizeof(*t));
}

static int fd_find_free(fd_table_t *t)
{
    for (int i = 0; i < FD_MAX; ++i)
    {
        if (!t->entries[i].used)
            return i;
    }

    return ERR_TOO_MANY_OPEN_FILES;
}

int fd_alloc(fd_table_t *t, struct file *f)
{
    if (!t || !f)
        return ERR_INVALID_ARGUMENT;

    int fd = fd_find_free(t);
    if (fd < 0)
        return fd;

    file_ref(f);
    t->entries[fd].file = f;
    t->entries[fd].used = true;
    return fd;
}

int fd_alloc_at(fd_table_t *t, int want_fd, struct file *f)
{
    if (!t || !f || want_fd < 0 || want_fd >= FD_MAX)
        return ERR_INVALID_ARGUMENT;

    if (t->entries[want_fd].used)
    {
        if (t->entries[want_fd].file == f)
            return want_fd;

        int result = fd_close(t, want_fd);
        if (result != ERR_SUCCESS)
            return result;
    }

    file_ref(f);
    t->entries[want_fd].file = f;
    t->entries[want_fd].used = true;
    return want_fd;
}

int fd_install_take(fd_table_t *t, struct file *f)
{
    if (!t || !f)
        return ERR_INVALID_ARGUMENT;

    int fd = fd_find_free(t);
    if (fd < 0)
        return fd;

    t->entries[fd].file = f;
    t->entries[fd].used = true;
    return fd;
}

struct file *fd_get(fd_table_t *t, int fd)
{
    if (!t || fd < 0 || fd >= FD_MAX || !t->entries[fd].used)
        return NULL;
    return t->entries[fd].file;
}

int fd_close(fd_table_t *t, int fd)
{
    if (!t || fd < 0 || fd >= FD_MAX || !t->entries[fd].used)
        return ERR_BAD_FILE_DESCRIPTOR;

    struct file *f = t->entries[fd].file;
    t->entries[fd].file = NULL;
    t->entries[fd].used = false;
    file_unref(f);
    return ERR_SUCCESS;
}

int fd_dup(fd_table_t *t, int oldfd)
{
    if (!t)
        return ERR_INVALID_ARGUMENT;
    struct file *f = fd_get(t, oldfd);
    if (!f)
        return ERR_BAD_FILE_DESCRIPTOR;
    return fd_alloc(t, f);
}

int fd_dup2(fd_table_t *t, int oldfd, int newfd)
{
    if (!t || newfd < 0 || newfd >= FD_MAX)
        return ERR_INVALID_ARGUMENT;
    struct file *f = fd_get(t, oldfd);
    if (!f)
        return ERR_BAD_FILE_DESCRIPTOR;
    if (oldfd == newfd)
        return newfd;
    return fd_alloc_at(t, newfd, f);
}

int fd_count(fd_table_t *t)
{
    if (!t)
        return -1;
    int c = 0;
    for (int i = 0; i < FD_MAX; ++i)
    {
        if (t->entries[i].used)
            ++c;
    }
    return c;
}