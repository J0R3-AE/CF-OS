#pragma once

#include "kernel/fs/vfs.h"

int vfs_resolve_parent_path(const char *path, struct vnode **parent_out,
                            char **leaf_name_out);
