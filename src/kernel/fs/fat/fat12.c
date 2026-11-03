#include "kernel/fs/fat.h"
#include "libc/errno.h"

static int fat12_get(struct fat_super *sb, u32 index, u32 *value)
{
    if (!value || index >= sb->cluster_count + 2)
        return ERR_INVALID_ARGUMENT;

    u32 fat_offset = index + (index / 2);
    if (fat_offset + 1 >= sb->fat_sectors * sb->bytes_per_sector)
        return ERR_IO_ERROR;

    u8 bytes[2];
    u32 sector = sb->fat_start + fat_offset / sb->bytes_per_sector;
    u32 offset = fat_offset % sb->bytes_per_sector;
    u8 buf[512];
    int result = blockdev_read(sb->bdev, sector, 1, buf);
    if (result != ERR_SUCCESS)
        return result;
    bytes[0] = buf[offset];
    if (offset + 1 < sb->bytes_per_sector)
        bytes[1] = buf[offset + 1];
    else
    {
        result = blockdev_read(sb->bdev, sector + 1, 1, buf);
        if (result != ERR_SUCCESS)
            return result;
        bytes[1] = buf[0];
    }

    u16 entry = (u16)bytes[0] | ((u16)bytes[1] << 8);
    if (index & 1)
        entry >>= 4;
    else
        entry &= 0x0FFF;
    *value = entry;
    return ERR_SUCCESS;
}

static int fat12_set(struct fat_super *sb, u32 index, u32 value)
{
    if (!sb || index >= sb->cluster_count + 2 || value > 0x0FFF ||
        sb->bytes_per_sector != 512)
        return ERR_INVALID_ARGUMENT;

    u32 offset = index + index / 2;
    if (offset + 1 >= sb->fat_sectors * sb->bytes_per_sector)
        return ERR_IO_ERROR;

    u8 sectors[2][512];
    for (u32 copy = 0; copy < sb->fat_count; copy++)
    {
        u32 fat_base = sb->fat_start + copy * sb->fat_sectors;
        u32 sector_index = offset / sb->bytes_per_sector;
        u32 in_sector = offset % sb->bytes_per_sector;
        u32 first_lba = fat_base + sector_index;
        int result = blockdev_read(sb->bdev, first_lba, 1, sectors[0]);
        if (result != ERR_SUCCESS)
            return result;

        if (in_sector == (u32)sb->bytes_per_sector - 1)
        {
            result = blockdev_read(sb->bdev, first_lba + 1, 1, sectors[1]);
            if (result != ERR_SUCCESS)
                return result;
        }

        u8 low = sectors[0][in_sector];
        u8 high = in_sector + 1 < sb->bytes_per_sector
                      ? sectors[0][in_sector + 1]
                      : sectors[1][0];
        u16 entry = (u16)low | ((u16)high << 8);
        if (index & 1)
            entry = (u16)((entry & 0x000F) | ((value & 0x0FFF) << 4));
        else
            entry = (u16)((entry & 0xF000) | (value & 0x0FFF));

        sectors[0][in_sector] = (u8)entry;
        if (in_sector + 1 < sb->bytes_per_sector)
            sectors[0][in_sector + 1] = (u8)(entry >> 8);
        else
            sectors[1][0] = (u8)(entry >> 8);

        result = blockdev_write(sb->bdev, first_lba, 1, sectors[0]);
        if (result != ERR_SUCCESS)
            return result;
        if (in_sector == (u32)sb->bytes_per_sector - 1)
        {
            result = blockdev_write(sb->bdev, first_lba + 1, 1, sectors[1]);
            if (result != ERR_SUCCESS)
                return result;
        }
    }

    return ERR_SUCCESS;
}

static u32 fat12_root(struct fat_super *sb)
{
    (void)sb;
    return 0; // special root
}

static int fat12_is_eoc(struct fat_super *sb, u32 value)
{
    (void)sb;
    return value >= 0x0FF8;
}

fat_variant_ops_t fat12_ops = {
    .get_fat_entry          = fat12_get,
    .set_fat_entry          = fat12_set,
    .root_dir_first_cluster = fat12_root,
    .is_eoc                 = fat12_is_eoc,
};
