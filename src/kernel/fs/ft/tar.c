#include "fs/vfs.h"

#include "libk/types.h"
#include "libk/string.h"
#include "libk/mem.h"
#include "libk/log.h"

#include "mm/heap.h"

struct tar_header
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
};

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

void tar_extract(struct vnode *root, void *start, u32 size)
{
    if (!root || !start || size == 0)
    {
        return;
    }

    KLOG_INFO("tar_extract: start root=%p size=%u", root, size);

    u8 *ptr = (u8 *)start;
    u8 *end = ptr + size;

    while (ptr < end)
    {
        if (ptr + 512 > end)
        {
            KLOG_ERROR("tar_extract: incomplete tar header at %p", ptr);
            break;
        }

        struct tar_header *hdr = (struct tar_header *)ptr;

        if (hdr->name[0] == '\0')
        {
            KLOG_INFO("tar_extract: reached end of archive at %p", ptr);
            break;
        }

        char name[128];
        memcpy(name, hdr->name, 100);
        name[100] = '\0';
        normalize(name);

        u32 fsize = oct2bin(hdr->size);
        KLOG_INFO("tar_extract: entry name=%s type=%c size=%u ptr=%p", name,
                  hdr->typeflag ? hdr->typeflag : '-', fsize, ptr);

        if (name[0] == '\0')
        {
            ptr += 512;
            continue;
        }

        char parent_path[128];
        char leaf[64];
        split_path(name, parent_path, leaf);

        struct vnode *parent =
            (parent_path[0] == '\0') ? root : mkdir_p(root, parent_path);

        if (!parent)
        {
            return;
        }

        struct vnode *node = NULL;

        if (hdr->typeflag == '5')
        {
            if (parent->ops->lookup(parent, leaf, &node) != 0)
            {
                parent->ops->create(parent, leaf, VNODE_TYPE_DIR, &node);
                KLOG_INFO("tar_extract: created dir %s", name);
            }
            else
            {
                KLOG_INFO("tar_extract: dir exists %s", name);
            }
        }
        else
        {
            KLOG_INFO("tar_extract: creating file %s", name);
            KLOG_INFO("tar_extract: parent=%p ops=%p", parent, parent ? parent->ops : NULL);
            int create_result = parent->ops->create(parent, leaf, VNODE_TYPE_FILE, &node);
            KLOG_INFO("tar_extract: create returned %d for %s", create_result, name);
            if (create_result != 0)
            {
                KLOG_ERROR("tar_extract: failed to create file %s", name);
            }
            else
            {
                KLOG_INFO("tar_extract: created file %s", name);
                KLOG_INFO("tar_extract: opening vnode for %s", name);
                struct file *f = vfs_open_vnode(node);
                if (f)
                {
                    KLOG_INFO("tar_extract: writing %u bytes to %s", fsize, name);
                    usize written;
                    int wr = vfs_write(f, ptr + 512, fsize, &written);
                    if (wr != 0)
                    {
                        KLOG_ERROR("tar_extract: vfs_write failed %d for %s", wr, name);
                    }
                    else
                    {
                        KLOG_INFO("tar_extract: wrote %u bytes to %s", written, name);
                    }
                    vfs_close(f);
                }
                else
                {
                    KLOG_ERROR("tar_extract: failed to open vnode for %s", name);
                }
            }
        }

        u32 total = ((fsize + 511) / 512) * 512;
        ptr += 512 + total;
        KLOG_INFO("tar_extract: next ptr=%p", ptr);
    }

}