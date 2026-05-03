#pragma once
/**
 * @file fat.h
 * @brief FAT12/16/32 filesystem structures and initialization.
 *
 * This header defines the core structures for the FAT filesystem driver:
 * - FAT superblock (parsed from the BPB)
 * - FAT variant operations (FAT12/16/32 differences)
 * - FAT vnode metadata
 *
 * FAT is a simple, widely‑supported filesystem used for boot partitions,
 * removable media, and compatibility layers. This implementation supports
 * read‑only or read/write depending on backend block device support.
 */

#include "fs_types.h"
#include "mount.h"
#include "vfs.h"
#include "libk/types.h"
#include "blockdev.h"

struct blockdev;
struct fat_super;

/* -------------------------------------------------------------------------- */
/*  FAT Type Enum                                                             */
/* -------------------------------------------------------------------------- */

/**
 * @enum fat_type_t
 * @brief FAT variant type.
 */
typedef enum
{
    FAT_TYPE_12, /**< FAT12 (floppies, tiny partitions). */
    FAT_TYPE_16, /**< FAT16 (small disks, DOS). */
    FAT_TYPE_32  /**< FAT32 (modern removable media). */
} fat_type_t;

/* -------------------------------------------------------------------------- */
/*  FAT Variant Operations                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @struct fat_variant_ops
 * @brief FAT12/16/32‑specific operations.
 *
 * FAT variants differ in:
 * - FAT entry width (12/16/32 bits)
 * - EOC (end‑of‑clusterchain) markers
 * - root directory handling (FAT32 uses a cluster chain)
 */
typedef struct fat_variant_ops
{
    u32 (*get_fat_entry)(struct fat_super *sb, u32 index);
    void (*set_fat_entry)(struct fat_super *sb, u32 index, u32 value);
    u32 (*root_dir_first_cluster)(struct fat_super *sb);
    int (*is_eoc)(struct fat_super *sb, u32 value);
} fat_variant_ops_t;

/* -------------------------------------------------------------------------- */
/*  FAT Superblock                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @struct fat_super
 * @brief Parsed FAT BIOS Parameter Block (BPB) and computed layout.
 *
 * Contains both raw BPB fields and computed offsets for:
 * - FAT region
 * - root directory region
 * - data region
 *
 * Also stores the FAT variant and block device pointer.
 */
typedef struct fat_super
{
    struct blockdev *bdev;  /**< Underlying block device. */
    fat_type_t type;        /**< FAT12/16/32. */
    fat_variant_ops_t *var; /**< Variant‑specific handlers. */

    /* BPB fields */
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    u8 fat_count;
    u16 root_entries;
    u16 total_sectors16;
    u32 total_sectors32;
    u16 sectors_per_fat16;
    u32 sectors_per_fat32;

    /* Computed layout */
    u32 fat_start;      /**< First FAT sector. */
    u32 root_dir_start; /**< First root directory sector (FAT12/16). */
    u32 data_start;     /**< First data sector. */
    u32 root_cluster;   /**< FAT32 root directory cluster. */
} fat_super_t;

/* -------------------------------------------------------------------------- */
/*  FAT Node                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @struct fat_node
 * @brief In‑memory representation of a FAT file or directory.
 *
 * Contains:
 * - first cluster of the file/directory
 * - file size
 * - attribute flags
 */
typedef struct fat_node
{
    fat_super_t *sb; /**< Filesystem superblock. */
    u32 first_cluster;
    u32 size;
    u8 attr;
} fat_node_t;

/* -------------------------------------------------------------------------- */
/*  FAT API                                                                   */
/* -------------------------------------------------------------------------- */

/// @brief Initialize and register the FAT filesystem driver.
void fat_init(void);

/* FAT_H */
