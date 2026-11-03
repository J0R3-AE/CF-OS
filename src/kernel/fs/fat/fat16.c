#include "kernel/fs/fat.h"
#include "libc/errno.h"

static int fat16_get(struct fat_super *sb, u32 index, u32 *value)
{
    if (!value || index >= sb->cluster_count + 2)
        return ERR_INVALID_ARGUMENT;

    u32 fat_offset = index * 2;
    if (fat_offset + 1 >= sb->fat_sectors * sb->bytes_per_sector)
        return ERR_IO_ERROR;

    u8 buf[512];
    int result = blockdev_read(sb->bdev,
                               sb->fat_start + fat_offset / sb->bytes_per_sector,
                               1, buf);
    if (result != ERR_SUCCESS)
        return result;

    u32 offset = fat_offset % sb->bytes_per_sector;
    *value = (u32)buf[offset] | ((u32)buf[offset + 1] << 8);
    return ERR_SUCCESS;
}

static int fat16_set(struct fat_super *sb, u32 index, u32 value)
{
    if (!sb || index >= sb->cluster_count + 2 ||
        value > 0xFFFF || sb->bytes_per_sector != sizeof(u8) * 512)
        return ERR_INVALID_ARGUMENT;

    u32 offset = index * 2;
    if (offset + 1 >= sb->fat_sectors * sb->bytes_per_sector)
        return ERR_IO_ERROR;

    u8 sector[512];
    for (u32 copy = 0; copy < sb->fat_count; copy++)
    {
        u32 lba = sb->fat_start + copy * sb->fat_sectors +
                  offset / sb->bytes_per_sector;
        int result = blockdev_read(sb->bdev, lba, 1, sector);
        if (result != ERR_SUCCESS)
            return result;

        u32 in_sector = offset % sb->bytes_per_sector;
        sector[in_sector] = (u8)value;
        sector[in_sector + 1] = (u8)(value >> 8);
        result = blockdev_write(sb->bdev, lba, 1, sector);
        if (result != ERR_SUCCESS)
            return result;
    }

    return ERR_SUCCESS;
}

static u32 fat16_root(struct fat_super *sb)
{
    (void)sb;
    return 0; // special root
}

static int fat16_is_eoc(struct fat_super *sb, u32 value)
{
    (void)sb;
    return value >= 0xFFF8;
}

fat_variant_ops_t fat16_ops = {
    .get_fat_entry          = fat16_get,
    .set_fat_entry          = fat16_set,
    .root_dir_first_cluster = fat16_root,
    .is_eoc                 = fat16_is_eoc,
};
