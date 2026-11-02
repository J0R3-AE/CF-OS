#pragma once

#include "kernel/fs/ramfs.h"

extern const struct vnode_ops ramfs_vnode_ops;

struct ramfs_node *ramfs_node_create(const char *name, vnode_type_t type);
struct vnode *ramfs_node_vnode(struct mount *mp, struct ramfs_node *node);
void ramfs_node_free(struct ramfs_node *node);
void ramfs_node_release(struct ramfs_node *node);
void ramfs_tree_release(struct ramfs_node *node);
void ramfs_destroy_vnode(struct vnode *vn);

int ramfs_lookup_vn(struct vnode *dir, const char *name, struct vnode **out);
int ramfs_create_vn(struct vnode *dir, const char *name, vnode_type_t type, struct vnode **out);
int ramfs_unlink_vn(struct vnode *dir, const char *name);
int ramfs_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out);
int ramfs_write_vn(struct vnode *vn, const void *buf, usize off, usize len, usize *out);
int ramfs_readdir_vn(struct vnode *vn, usize index, const char **name_out, vnode_type_t *type_out);
int ramfs_truncate_vn(struct vnode *vn, usize new_size);
int ramfs_getattr_vn(struct vnode *vn, void *stat_buf);
int ramfs_setattr_vn(struct vnode *vn, const void *stat_buf);
