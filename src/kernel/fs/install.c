/**
 * @file kernel/fs/install.c
 * @brief Disk install and bootstrap utilities.
 */

#include "kernel/fs/install.h"
#include "kernel/fs/mount.h"
#include "kernel/fs/vfs.h"
#include "kernel/fs/blockdev.h"
#include "kernel/fs/partition.h"
#include "kernel/drivers/ata.h"
#include "kernel/log.h"
#include "libc/errno.h"
#include "libc/string.h"
#include "libc/mem.h"

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
    struct blockdev *bdev = blockdev_lookup("hd0");
    if (!bdev)
    {
        KLOG_WARN("install_mount_disk_root: no ATA device found");
        return -1;
    }

    int partition_count = partition_scan(bdev);
    if (partition_count < 0 && partition_count != ERR_NOT_FOUND)
    {
        KLOG_WARN("install_mount_disk_root: partition scan failed (%d)",
                  partition_count);
        return partition_count;
    }

    for (u32 i = 1; i <= 4; i++)
    {
        char partition_name[] = "hd0p0";
        partition_name[4] = (char)('0' + i);
        if (!blockdev_lookup(partition_name))
            continue;

        KLOG_INFO("install_mount_disk_root: trying %s", partition_name);
        if (mount_do_mount(&fat_type, partition_name, "/", NULL) == 0)
        {
            KLOG_OKAY("install_mount_disk_root: FAT partition %s mounted",
                      partition_name);
            return 0;
        }
        if (mount_do_mount(&ext2_type, partition_name, "/", NULL) == 0)
        {
            KLOG_OKAY("install_mount_disk_root: EXT2 partition %s mounted",
                      partition_name);
            return 0;
        }
    }

    /* Support superfloppy images that contain a filesystem without an MBR. */
    if (mount_do_mount(&fat_type, "hd0", "/", NULL) == 0 ||
        mount_do_mount(&ext2_type, "hd0", "/", NULL) == 0)
    {
        KLOG_OKAY("install_mount_disk_root: whole-disk filesystem mounted");
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
 * Disk formatting and file copying are not implemented yet.
 */
int install_format_and_copy(void)
{
    KLOG_WARN("install_format_and_copy: disk installation is not supported yet");
    return ERR_NOT_SUPPORTED;
}
