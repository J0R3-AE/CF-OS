#include "fs/fat.h"

static u32 fat16_get(struct fat_super *sb, u32 index)
{
    u32 fat_offset = index * 2;
    u32 fat_sector = sb->fat_start + (fat_offset / sb->bytes_per_sector);
    u32 ent_offset = fat_offset % sb->bytes_per_sector;

    u8 buf[512];
    sb->bdev->read(sb->bdev, fat_sector, 1, buf);

    u16 val = *(u16 *)(buf + ent_offset);
    return val;
}

static void fat16_set(struct fat_super *sb, u32 index, u32 value)
{
    (void)sb; (void)index; (void)value;
    // TODO
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
