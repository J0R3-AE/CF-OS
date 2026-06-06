/**
 * @file kernel/fs/install.c
 * @brief Disk install and bootstrap utilities.
 */

#include "fs/install.h"
#include "fs/mount.h"
#include "fs/vfs.h"
#include "fs/blockdev.h"
#include "drivers/ata.h"
#include "libk/log.h"
#include "libk/string.h"
#include "libk/mem.h"

extern struct fs_type ramfs_type;
extern struct fs_type fat_type;
extern struct fs_type ext2_type;

/**
 * @brief Check if a filesystem exists on disk and mount it.
 *
 * Tries to mount the primary ATA device as various filesystem types.
 * Currently tries FAT12/16/32 and EXT2.
 */
int install_mount_disk_root(void)
{
    struct blockdev *bd = blockdev_open("ata0");
    if (!bd)
    {
        KLOG_WARN("install_mount_disk_root: no ATA device found");
        return -1;
    }

    KLOG_INFO("install_mount_disk_root: trying to mount disk root");

    /* Try FAT filesystem first */
    if (mount_do_mount(&fat_type, "ata0", "/", NULL) == 0)
    {
        KLOG_OKAY("install_mount_disk_root: FAT root mounted successfully");
        return 0;
    }

    KLOG_WARN("install_mount_disk_root: FAT mount failed, trying EXT2");

    /* Try EXT2 filesystem */
    if (mount_do_mount(&ext2_type, "ata0", "/", NULL) == 0)
    {
        KLOG_OKAY("install_mount_disk_root: EXT2 root mounted successfully");
        return 0;
    }

    KLOG_WARN("install_mount_disk_root: no valid filesystem found on disk");
    return -1;
}

/**
 * @brief Check if disk already has a valid installation.
 */
int install_check_disk_installed(void)
{
    struct vnode *init_vn = NULL;

    /* If we already mounted disk root, check for /init */
    if (vfs_lookup("/init", &init_vn) == 0 && init_vn)
    {
        KLOG_INFO("install_check_disk_installed: /init found on disk");
        return 1;
    }

    KLOG_INFO("install_check_disk_installed: /init not found on disk");
    return 0;
}

/**
 * @brief Format disk with FAT32 and copy root files.
 *
 * This is a stub implementation that will be expanded later.
 * For now, it just logs that install mode is needed.
 */
int install_format_and_copy(void)
{
    KLOG_INFO("install_format_and_copy: disk format/install not yet fully implemented");
    KLOG_INFO("install_format_and_copy: starting installer process to handle this");
    return 0;
}
