#pragma once
#include "vnode.h"
#include "libk/link.h"

struct mount;
struct fs_type;

// Filesystem Operations
struct fs_ops
{
    int (*mount)(struct mount *mp, const char *opts);
    int (*unmount)(struct mount *mp);
    int (*sync)(struct mount *mp);
};

// Filesystem Type
struct fs_type
{
    Link link;
    const char *name;
    const struct fs_ops *fs_ops;
    int (*root_vnode)(struct mount *mp, struct vnode **out_root);
};