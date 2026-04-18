#include "fs/fd.h"
#include "fs/vfs.h"    /* for vfs_close / file struct */
#include <libk/string.h> /* for memset if your kernel provides it */

/* If you don't have kmalloc/kfree, define simple wrappers or use a static pool.
 * For now we assume kmalloc/kfree exist.
 */

/* Simple helper: return negative error codes */
#ifndef EMFILE
#define EMFILE -24 /* too many open files */
#endif
#ifndef EINVAL
#define EINVAL -22
#endif
#ifndef EOK
#define EOK 0
#endif

void fd_table_init(fd_table_t *t)
{
    if (!t)
        return;
    /* kernel-friendly memset: if you have bzero/memset */
    memset(t, 0, sizeof(*t));
}

/* find free slot; return index or -1 */
static int fd_find_free(fd_table_t *t)
{
    if (!t)
        return -1;
    for (int i = 0; i < FD_MAX; ++i)
    {
        if (!t->entries[i].used)
            return i;
    }
    return -1;
}

/* allocate FD and take ownership: increments file refcount */
int fd_alloc(fd_table_t *t, struct file *f)
{
    if (!t || !f)
        return EINVAL;
    int fd = fd_find_free(t);
    if (fd < 0)
        return EMFILE;

    /* add another reference for the fd */
    file_ref(f);

    t->entries[fd].file = f;
    t->entries[fd].used = true;
    return fd;
}

/* install at specific fd (dup2 semantics) */
int fd_alloc_at(fd_table_t *t, int want_fd, struct file *f)
{
    if (!t || !f)
        return EINVAL;
    if (want_fd < 0 || want_fd >= FD_MAX)
        return EINVAL;

    /* If target in use, close it first */
    if (t->entries[want_fd].used)
    {
        fd_close(t, want_fd);
    }

    file_ref(f);
    t->entries[want_fd].file = f;
    t->entries[want_fd].used = true;
    return want_fd;
}

/* take ownership (does NOT increment ref) - caller transferred ownership */
int fd_install_take(fd_table_t *t, struct file *f)
{
    if (!t || !f)
        return EINVAL;
    int fd = fd_find_free(t);
    if (fd < 0)
        return EMFILE;
    t->entries[fd].file = f;
    t->entries[fd].used = true;
    return fd;
}

struct file *fd_get(fd_table_t *t, int fd)
{
    if (!t)
        return NULL;
    if (fd < 0 || fd >= FD_MAX)
        return NULL;
    if (!t->entries[fd].used)
        return NULL;
    return t->entries[fd].file;
}

/* Close fd: unmap and unref file (file_unref calls vfs_close when ref hits 0) */
int fd_close(fd_table_t *t, int fd)
{
    if (!t)
        return EINVAL;
    if (fd < 0 || fd >= FD_MAX)
        return EINVAL;
    if (!t->entries[fd].used)
        return EINVAL;

    struct file *f = t->entries[fd].file;
    t->entries[fd].file = NULL;
    t->entries[fd].used = false;

    /* release the fd's reference */
    file_unref(f);
    return EOK;
}

/* Duplicate fd -> newfd (allocates new fd and increments refcount) */
int fd_dup(fd_table_t *t, int oldfd)
{
    if (!t)
        return EINVAL;
    struct file *f = fd_get(t, oldfd);
    if (!f)
        return EINVAL;
    return fd_alloc(t, f); /* fd_alloc will file_ref */
}

/* Duplicate oldfd into newfd (dup2-like) */
int fd_dup2(fd_table_t *t, int oldfd, int newfd)
{
    if (!t)
        return EINVAL;
    struct file *f = fd_get(t, oldfd);
    if (!f)
        return EINVAL;
    return fd_alloc_at(t, newfd, f);
}

int fd_count(fd_table_t *t)
{
    if (!t)
        return 0;
    int c = 0;
    for (int i = 0; i < FD_MAX; ++i)
        if (t->entries[i].used)
            ++c;
    return c;
}