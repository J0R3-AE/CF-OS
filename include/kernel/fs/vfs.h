#pragma once
/**
 * @file vfs.h
 * @brief Virtual File System (VFS) core interfaces.
 *
 * The VFS provides a unified interface over filesystem implementations
 * such as RAMFS, EXT2, FAT, and future filesystems.
 *
 * Responsibilities:
 * - filesystem registration
 * - mounting/unmounting
 * - path resolution
 * - file operations
 * - vnode management
 */

#include "fs_types.h"
#include "file.h"
#include "vnode.h"
#include "libc/link.h"


// Open Flags           

#define VFS_O_RDONLY 0x1
#define VFS_O_WRONLY 0x2
#define VFS_O_RDWR   (VFS_O_RDONLY | VFS_O_WRONLY)
#define VFS_O_CREATE 0x4

// Stat                 
typedef struct vfs_stat
{
    vnode_type_t type;

    usize size;
    usize capacity;

    u32 mode;
    u32 uid;
    u32 gid;

    u32 atime;
    u32 mtime;
    u32 ctime;

} vfs_stat_t;


// Filesystem Management


/**
 * Register filesystem driver.
 */
int vfs_register_fs(struct fs_type *fst);


/**
 * Mount filesystem.
 */
int vfs_mount(const char *fs_name,
              const char *source,
              const char *target,
              const char *opts);


/**
 * Unmount filesystem.
 */
int vfs_unmount(const char *target);


// File Operations      


/**
 * Open file by path.
 */
int vfs_open(const char *path,
             u32 flags,
             struct file **out);


/**
 * Open vnode directly.
 */
struct file *vfs_open_vnode(struct vnode *vn);


/**
 * Close file.
 */
int vfs_close(struct file *f);


/**
 * Read file.
 */
int vfs_read(struct file *f,
             void *buf,
             usize len,
             usize *out);


/**
 * Write file.
 */
int vfs_write(struct file *f,
              const void *buf,
              usize len,
              usize *out);


/**
 * Change file offset.
 */
int vfs_seek(struct file *f,
             usize offset);


/**
 * Read directory entry.
 */
int vfs_readdir(struct vnode *vn,
                usize index,
                const char **name_out,
                vnode_type_t *type_out);


/**
 * Get file information.
 */
int vfs_stat(const char *path,
             vfs_stat_t *st);


/**
 * Create directory.
 */
int vfs_mkdir(const char *path);


/**
 * Remove file or empty directory.
 */
int vfs_unlink(const char *path);


// Path Resolution      


/**
 * Resolve path to vnode.
 */
int vfs_lookup(const char *path,
               struct vnode **out);


/**
 * Resolve path.
 *
 * Returns vnode directly.
 */
struct vnode *vfs_resolve(const char *path);


/**
 * Resolve parent directory and final component.
 *
 * Used by create/unlink/mkdir.
 */
struct vnode *vfs_resolve_path(
        const char *path,
        struct vnode **parent_out,
        const char **leaf_name_out);


/**
 * Change process root vnode.
 */
int vfs_chroot(struct vnode *new_root);


/**
 * Set global filesystem root.
 */
int vfs_set_root(struct vnode *root);


/**
 * Get global root vnode.
 */
struct vnode *vfs_get_root(void);


// Executable Helpers   


/**
 * Create executable vnode.
 */
int vfs_create_exec(const char *path,
                    exec_fn_t fn);


// Reference Counting   

void file_ref(struct file *f);

void file_unref(struct file *f);


// VFS_H */