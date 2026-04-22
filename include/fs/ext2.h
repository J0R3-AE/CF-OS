#pragma once
/**
 * @file ext2.h
 * @brief EXT2 filesystem structures and initialization.
 *
 * This header defines the on‑disk EXT2 structures (superblock, inode,
 * block group descriptor) and the in‑memory node representation used by
 * the VFS layer. Only minimal EXT2 functionality is exposed here; the
 * implementation lives in ext2.c.
 */

#include "fs_types.h"
#include "mount.h"
#include "vfs.h"
#include "libk/types.h"
#include "blockdev.h"

struct blockdev; /* Forward declaration */

/* -------------------------------------------------------------------------- */
/*  EXT2 Superblock                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @struct ext2_super
 * @brief In‑memory representation of the EXT2 superblock.
 *
 * This structure mirrors the on‑disk EXT2 superblock layout, with a few
 * additional computed fields (block_size, groups_count, bgdt_block).
 */
typedef struct ext2_super
{
    struct blockdev *bdev; /**< Underlying block device. */

    u32 inodes_count;
    u32 blocks_count;
    u32 r_blocks_count;
    u32 free_blocks_count;
    u32 free_inodes_count;
    u32 first_data_block;
    u32 log_block_size;
    u32 log_frag_size;
    u32 blocks_per_group;
    u32 frags_per_group;
    u32 inodes_per_group;
    u32 mtime;
    u32 wtime;

    u16 mount_count;
    u16 max_mount_count;
    u16 magic;
    u16 state;
    u16 errors;
    u16 minor_rev_level;
    u32 lastcheck;
    u32 checkinterval;
    u32 creator_os;
    u32 rev_level;
    u16 def_resuid;
    u16 def_resgid;

    u32 first_ino;
    u16 inode_size;
    u16 block_group_nr;
    u32 feature_compat;
    u32 feature_incompat;
    u32 feature_ro_compat;

    /* Computed fields (not part of the on‑disk superblock) */
    u32 block_size;   /**< Block size in bytes. */
    u32 groups_count; /**< Number of block groups. */
    u32 bgdt_block;   /**< Block containing the block group descriptor table. */
} ext2_super_t;

/* -------------------------------------------------------------------------- */
/*  EXT2 Inode                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @struct ext2_inode
 * @brief On‑disk EXT2 inode structure.
 *
 * Contains metadata and block pointers for files and directories.
 */
typedef struct ext2_inode
{
    u16 mode;
    u16 uid;
    u32 size;
    u32 atime;
    u32 ctime;
    u32 mtime;
    u32 dtime;
    u16 gid;
    u16 links_count;
    u32 blocks;
    u32 flags;
    u32 osd1;
    u32 block[15]; /**< Direct, indirect, double‑indirect, triple‑indirect. */
    u32 generation;
    u32 file_acl;
    u32 dir_acl;
    u32 faddr;
    u8 osd2[12];
} ext2_inode_disk_t;

/* -------------------------------------------------------------------------- */
/*  Block Group Descriptor                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @struct ext2_bg_desc
 * @brief EXT2 block group descriptor.
 *
 * Describes the location of block/inode bitmaps and inode tables for each
 * block group.
 */
typedef struct ext2_bg_desc
{
    u32 block_bitmap;
    u32 inode_bitmap;
    u32 inode_table;
    u16 free_blocks_count;
    u16 free_inodes_count;
    u16 used_dirs_count;
    u16 pad;
    u8 reserved[12];
} ext2_bg_desc_t;

/* -------------------------------------------------------------------------- */
/*  EXT2 VFS Node                                                             */
/* -------------------------------------------------------------------------- */

/**
 * @struct ext2_node
 * @brief In‑memory representation of an EXT2 inode for VFS integration.
 */
typedef struct ext2_node
{
    ext2_super_t *sb;        /**< Pointer to the filesystem superblock. */
    u32 ino;                 /**< Inode number. */
    ext2_inode_disk_t inode; /**< Cached on‑disk inode. */
} ext2_node_t;

/* -------------------------------------------------------------------------- */
/*  EXT2 API                                                                  */
/* -------------------------------------------------------------------------- */

/// @brief Initialize the EXT2 filesystem driver.
void ext2_init(void);
