#include "fs/fat.h"

static u32 fat12_get(struct fat_super *sb, u32 index)
{
    u32 fat_offset = index + (index / 2);
    u32 fat_sector = sb->fat_start + (fat_offset / sb->bytes_per_sector);
    u32 ent_offset = fat_offset % sb->bytes_per_sector;

    u8 buf[512];
    sb->bdev->read(sb->bdev, fat_sector, 1, buf);

    u16 val = *(u16 *)(buf + ent_offset);
    if (index & 1)
        val >>= 4;
    else
        val &= 0x0FFF;
    return val;
}

static void fat12_set(struct fat_super *sb, u32 index, u32 value)
{
    (void)sb; (void)index; (void)value;
    // TODO
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
