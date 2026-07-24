#pragma once
/**
 * @file blockdev.h
 * @brief Generic block device layer.
 */

#include "libc/types.h"

struct blockdev;

/* -------------------------------------------------------------------------- */
/* Block Operations                                                           */
/* -------------------------------------------------------------------------- */

struct blockdev_ops
{
    int (*read)(struct blockdev *bd,
                u32 lba,
                u32 count,
                void *buf);

    int (*write)(struct blockdev *bd,
                 u32 lba,
                 u32 count,
                 const void *buf);
};

/* -------------------------------------------------------------------------- */
/* Block Device                                                               */
/* -------------------------------------------------------------------------- */

struct blockdev
{
    const char *name;

    const struct blockdev_ops *ops;

    u32 sector_size;

    void *private_data;

    struct blockdev *next;
};

/* -------------------------------------------------------------------------- */
/* Registration                                                               */
/* -------------------------------------------------------------------------- */

int blockdev_register(struct blockdev *bd);

int blockdev_unregister(struct blockdev *bd);

/* -------------------------------------------------------------------------- */
/* Lookup                                                                      */
/* -------------------------------------------------------------------------- */

struct blockdev *blockdev_lookup(const char *name);

/* -------------------------------------------------------------------------- */
/* Helpers                                                                     */
/* -------------------------------------------------------------------------- */

int blockdev_read(struct blockdev *bd,
                  u32 lba,
                  u32 count,
                  void *buf);

int blockdev_write(struct blockdev *bd,
                   u32 lba,
                   u32 count,
                   const void *buf);