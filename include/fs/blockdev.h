#pragma once
/**
 * @file blockdev.h
 * @brief Block device abstraction layer.
 *
 * A block device provides sector‑based read/write access to storage.
 * Filesystems (FAT, EXT2, etc.) operate on top of this interface.
 *
 * Each block device implementation supplies:
 * - a read function
 * - a write function (optional for read‑only devices)
 * - a sector size
 * - an opaque private data pointer
 *
 * Examples of block devices:
 * - RAM disk
 * - ATA/IDE drive
 * - AHCI/SATA device
 * - Virtual disk image
 */

#include <libk/types.h>

/* -------------------------------------------------------------------------- */
/*  Block Device Interface                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @struct blockdev
 * @brief Generic block device descriptor.
 *
 * All block devices must implement:
 * - read(): read `count` sectors starting at LBA into `buf`
 * - write(): write `count` sectors starting at LBA from `buf`
 *
 * Sector size is provided so filesystems can compute offsets correctly.
 */
struct blockdev
{
    int (*read)(struct blockdev *bd,
                u32 lba,
                u32 count,
                void *buf);

    int (*write)(struct blockdev *bd,
                 u32 lba,
                 u32 count,
                 const void *buf);

    u32 sector_size; /**< Size of a sector in bytes. */
    void *priv;      /**< Device‑specific data. */
};

/* -------------------------------------------------------------------------- */
/*  Block Device Lookup                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Open a block device by name.
 *
 * The interpretation of `name` is system‑defined:
 * - "ram0" for RAM disk
 * - "hd0" for first ATA disk
 * - "sd0" for first SCSI/AHCI disk
 *
 * @param name Device name.
 * @return Pointer to blockdev or NULL if not found.
 */
struct blockdev *blockdev_open(const char *name);

/* BLOCKDEV_H */
