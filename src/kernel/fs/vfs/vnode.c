#include "fs/vnode.h"
#include "mm/heap.h"

struct vnode *vnode_create(struct mount *mp, vnode_type_t type, const struct vnode_ops *ops, void *fs_data)
{
    struct vnode *vn = calloc(1, sizeof(*vn));

    if (!vn)
        return NULL;

    vn->type = type;
    vn->refcnt = 1;
    vn->mount = mp;
    vn->ops = ops;
    vn->fs_data = fs_data;
    vn->exec = NULL;

    return vn;
}

void vnode_ref(struct vnode *vn)
{
    if (!vn)
        return;

    vn->refcnt++;
}

void vnode_unref(struct vnode *vn)
{
    if (!vn)
        return;

    if (vn->refcnt == 0)
        return;

    vn->refcnt--;

    if (vn->refcnt == 0)
    {
        if (vn->ops && vn->ops->destroy)
            vn->ops->destroy(vn);

        free(vn);
    }
}