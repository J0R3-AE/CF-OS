#include "fs/fat.h"

static u32 fat32_get(struct fat_super *sb, u32 index)
{
    u32 fat_offset = index * 4;
    u32 fat_sector = sb->fat_start + (fat_offset / sb->bytes_per_sector);
    u32 ent_offset = fat_offset % sb->bytes_per_sector;

    u8 buf[512];
    sb->bdev->read(sb->bdev, fat_sector, 1, buf);

    u32 val = *(u32 *)(buf + ent_offset);
    return val & 0x0FFFFFFF;
}

static void fat32_set(struct fat_super *sb, u32 index, u32 value)
{
    (void)sb;
    (void)index;
    (void)value;
    // TODO
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
