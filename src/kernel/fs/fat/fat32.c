#include "kernel/fs/fat.h"
#include "libc/errno.h"

static int fat32_get(struct fat_super *sb, u32 index, u32 *value)
{
    if (!value || index >= sb->cluster_count + 2)
        return ERR_INVALID_ARGUMENT;

    u32 fat_offset = index * 4;
    if (fat_offset + 3 >= sb->fat_sectors * sb->bytes_per_sector)
        return ERR_IO_ERROR;

    u8 buf[512];
    int result = blockdev_read(sb->bdev,
                               sb->fat_start + fat_offset / sb->bytes_per_sector,
                               1, buf);
    if (result != ERR_SUCCESS)
        return result;

    u32 offset = fat_offset % sb->bytes_per_sector;
    *value = ((u32)buf[offset]) |
             ((u32)buf[offset + 1] << 8) |
             ((u32)buf[offset + 2] << 16) |
             ((u32)buf[offset + 3] << 24);
    *value &= 0x0FFFFFFF;
    return ERR_SUCCESS;
}

static int fat32_set(struct fat_super *sb, u32 index, u32 value)
{
    if (!sb || index >= sb->cluster_count + 2 ||
        sb->bytes_per_sector != 512)
        return ERR_INVALID_ARGUMENT;

    u32 offset = index * 4;
    if (offset + 3 >= sb->fat_sectors * sb->bytes_per_sector)
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
        value = (value & 0x0FFFFFFF) |
                (((u32)sector[in_sector + 3] & 0xF0) << 24);
        sector[in_sector] = (u8)value;
        sector[in_sector + 1] = (u8)(value >> 8);
        sector[in_sector + 2] = (u8)(value >> 16);
        sector[in_sector + 3] = (u8)(value >> 24);
        result = blockdev_write(sb->bdev, lba, 1, sector);
        if (result != ERR_SUCCESS)
            return result;
    }

    return ERR_SUCCESS;
}

static u32 fat32_root(struct fat_super *sb)
{
    return sb->root_cluster;
}

static int fat32_is_eoc(struct fat_super *sb, u32 value)
{
    (void)sb;
    return (value & 0x0FFFFFFF) >= 0x0FFFFFF8;
}

fat_variant_ops_t fat32_ops = {
    .get_fat_entry = fat32_get,
    .set_fat_entry = fat32_set,
    .root_dir_first_cluster = fat32_root,
    .is_eoc = fat32_is_eoc,
};
