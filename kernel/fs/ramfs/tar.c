#include "fs/vfs.h"
#include "fs/tar.h"
#include "libk/string.h"
#include "libk/log.h"
#include "libk/types.h"
#include "mm/heap.h"

/* ---------------- Helpers ---------------- */

static u32 oct2bin(const char *str)
{
    u32 val = 0;
    while (*str >= '0' && *str <= '7')
        val = (val << 3) + (*str++ - '0');
    return val;
}

static void normalize(char *name)
{
    if (name[0] == '.' && name[1] == '/')
        memmove(name, name + 2, strlen(name + 2) + 1);

    while (name[0] == '/')
        memmove(name, name + 1, strlen(name) + 1);

    size_t len = strlen(name);
    while (len > 0 && name[len - 1] == '/')
        name[--len] = '\0';
}

/* mkdir relative to root vnode */
static struct vnode *mkdir_p(struct vnode *root, const char *path)
{
    struct vnode *cur = root;
    if (!cur)
        return NULL;

    char tmp[128];
    strcpy(tmp, path);

    char *p = tmp;

    while (*p)
    {
        char *slash = strchr(p, '/');
        if (slash)
            *slash = '\0';

        if (*p)
        {
            struct vnode *next;

            if (cur->ops->lookup(cur, p, &next) != 0)
            {
                if (cur->ops->create(cur, p, VNODE_TYPE_DIR, &next) != 0)
                    return NULL;
            }

            cur = next;
        }

        if (!slash)
            break;

        *slash = '/';
        p = slash + 1;
    }

    return cur;
}

static void split_path(const char *path, char *parent, char *leaf)
{
    const char *last = strrchr(path, '/');

    if (!last)
    {
        parent[0] = '\0';
        strcpy(leaf, path);
        return;
    }

    size_t len = last - path;
    memcpy(parent, path, len);
    parent[len] = '\0';

    strcpy(leaf, last + 1);
}

/* ---------------- MAIN ---------------- */

int tar_extract(struct vnode *root, void *start, u32 size)
{
    if (!root || !start || size == 0)
    {
        klog_err("tar: invalid args");
        return;
    }

    u8 *ptr = (u8 *)start;
    u8 *end = ptr + size;

    while (ptr < end)
    {
        struct tar_header *hdr = (struct tar_header *)ptr;

        if (hdr->name[0] == '\0')
            break;

        char name[128];
        memcpy(name, hdr->name, 100);
        name[100] = '\0';

        normalize(name);

        if (name[0] == '\0')
        {
            ptr += 512;
            continue;
        }

        u32 fsize = oct2bin(hdr->size);

        char parent_path[128];
        char leaf[64];
        split_path(name, parent_path, leaf);

        struct vnode *parent =
            (parent_path[0] == '\0') ? root : mkdir_p(root, parent_path);

        if (!parent)
        {
            klog_err("tar: mkdir failed for %s", name);
            return;
        }

        struct vnode *node = NULL;

        if (hdr->typeflag == '5')
        {
            if (parent->ops->lookup(parent, leaf, &node) != 0)
            {
                parent->ops->create(parent, leaf, VNODE_TYPE_DIR, &node);
            }
        }
        else
        {
            if (parent->ops->create(parent, leaf, VNODE_TYPE_FILE, &node) != 0)
            {
                klog_warn("tar: create failed %s", name);
            }
            else
            {
                struct file *f = vfs_open_vnode(node);
                if (f)
                {
                    usize written;
                    vfs_write(f, ptr + 512, fsize, &written);
                    vfs_close(f);
                }
            }
        }

        u32 total = ((fsize + 511) / 512) * 512;
        ptr += 512 + total;
    }

    klog_info("tar: extraction complete");
}