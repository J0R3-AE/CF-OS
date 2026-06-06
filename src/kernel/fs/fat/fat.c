#include "fs/fat.h"

#include "libk/string.h"
#include "libk/mem.h"
#include "libk/types.h"
#include "libk/printf.h"

#include "mm/heap.h"

extern struct blockdev *blockdev_open(const char *name);

extern fat_variant_ops_t fat12_ops;
extern fat_variant_ops_t fat16_ops;
extern fat_variant_ops_t fat32_ops;

static int fat_mount_fn(struct mount *mp, const char *opts);
static int fat_unmount_fn(struct mount *mp);
static int fat_sync_fn(struct mount *mp);

static int fat_lookup_vn(struct vnode *dir, const char *name, struct vnode **out);
static int fat_create_vn(struct vnode *dir, const char *name, vnode_type_t type, struct vnode **out);
static int fat_unlink_vn(struct vnode *dir, const char *name);
static int fat_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out);
static int fat_write_vn(struct vnode *vn, const void *buf, usize off, usize len, usize *out);
static int fat_readdir_vn(struct vnode *vn, usize index, const char **name_out, vnode_type_t *type_out);
static int fat_truncate_vn(struct vnode *vn, usize new_size);
static int fat_getattr_vn(struct vnode *vn, void *stat_buf);
static int fat_setattr_vn(struct vnode *vn, const void *stat_buf);

static const struct fs_ops fat_fs_ops = {
    .mount = fat_mount_fn,
    .unmount = fat_unmount_fn,
    .sync = fat_sync_fn,
};

static const struct vnode_ops fat_vnode_ops = {
    .lookup = fat_lookup_vn,
    .create = fat_create_vn,
    .unlink = fat_unlink_vn,
    .read = fat_read_vn,
    .write = fat_write_vn,
    .readdir = fat_readdir_vn,
    .truncate = fat_truncate_vn,
    .getattr = fat_getattr_vn,
    .setattr = fat_setattr_vn,
};

struct fs_type fat_type = {
    .name = "fat",
    .fs_ops = &fat_fs_ops,
    .root_vnode = NULL
};

void fat_init(void)
{
    vfs_register_fs(&fat_type);
}

// ---- helpers ----

static int fat_read_sector(fat_super_t *sb, u32 lba, void *buf)
{
    return sb->bdev->read(sb->bdev, lba, 1, buf);
}

static fat_type_t fat_probe_type(fat_super_t *sb)
{
    u32 root_dir_sectors = ((sb->root_entries * 32) + (sb->bytes_per_sector - 1)) / sb->bytes_per_sector;
    u32 total_sectors = sb->total_sectors16 ? sb->total_sectors16 : sb->total_sectors32;
    u32 data_sectors = total_sectors - (sb->reserved_sectors + sb->fat_count * (sb->sectors_per_fat16 ? sb->sectors_per_fat16 : sb->sectors_per_fat32) + root_dir_sectors);
    u32 clusters = data_sectors / sb->sectors_per_cluster;

    if (clusters < 4085)
        return FAT_TYPE_12;
    if (clusters < 65525)
        return FAT_TYPE_16;
    return FAT_TYPE_32;
}

static struct vnode *fat_alloc_vnode(struct mount *mp, fat_super_t *sb, u32 first_cluster, u32 size, u8 attr)
{
    struct vnode *vn = (struct vnode *)malloc(sizeof(*vn));
    memset(vn, 0, sizeof(*vn));
    vn->type = (attr & 0x10) ? VNODE_TYPE_DIR : VNODE_TYPE_FILE;
    vn->mount = mp;
    vn->ops = &fat_vnode_ops;
    vn->refcnt = 1;

    fat_node_t *node = (fat_node_t *)malloc(sizeof(*node));
    node->sb = sb;
    node->first_cluster = first_cluster;
    node->size = size;
    node->attr = attr;

    vn->fs_data = node;
    return vn;
}

static u32 fat_cluster_to_lba(fat_super_t *sb, u32 cluster)
{
    return sb->data_start + (cluster - 2) * sb->sectors_per_cluster;
}

static int fat_read_cluster(fat_super_t *sb, u32 cluster, void *buf)
{
    u32 lba = fat_cluster_to_lba(sb, cluster);
    return sb->bdev->read(sb->bdev, lba, sb->sectors_per_cluster, buf);
}

// ---- mount ----

static int fat_mount_fn(struct mount *mp, const char *opts)
{
    struct blockdev *bdev = blockdev_open(mp->source);
    if (!bdev)
        return -1;

    fat_super_t *sb = (fat_super_t *)malloc(sizeof(*sb));
    memset(sb, 0, sizeof(*sb));
    sb->bdev = bdev;

    u8 bpb[512];
    bdev->read(bdev, 0, 1, bpb);

    memcpy(&sb->bytes_per_sector, bpb + 11, 2);
    memcpy(&sb->sectors_per_cluster, bpb + 13, 1);
    memcpy(&sb->reserved_sectors, bpb + 14, 2);
    memcpy(&sb->fat_count, bpb + 16, 1);
    memcpy(&sb->root_entries, bpb + 17, 2);
    memcpy(&sb->total_sectors16, bpb + 19, 2);
    memcpy(&sb->sectors_per_fat16, bpb + 22, 2);
    memcpy(&sb->total_sectors32, bpb + 32, 4);
    memcpy(&sb->sectors_per_fat32, bpb + 36, 4);
    memcpy(&sb->root_cluster, bpb + 44, 4);

    sb->type = fat_probe_type(sb);

    u32 fat_size = sb->sectors_per_fat16 ? sb->sectors_per_fat16 : sb->sectors_per_fat32;
    u32 root_dir_sectors = ((sb->root_entries * 32) + (sb->bytes_per_sector - 1)) / sb->bytes_per_sector;

    sb->fat_start = sb->reserved_sectors;
    if (sb->type == FAT_TYPE_32)
    {
        sb->root_dir_start = sb->fat_start + sb->fat_count * fat_size;
        sb->data_start = sb->root_dir_start;
    }
    else
    {
        sb->root_dir_start = sb->fat_start + sb->fat_count * fat_size;
        sb->data_start = sb->root_dir_start + root_dir_sectors;
    }

    switch (sb->type)
    {
    case FAT_TYPE_12:
        sb->var = &fat12_ops;
        break;
    case FAT_TYPE_16:
        sb->var = &fat16_ops;
        break;
    case FAT_TYPE_32:
        sb->var = &fat32_ops;
        break;
    }

    mp->fs_data = sb;

    struct vnode *root;
    if (sb->type == FAT_TYPE_32)
    {
        root = fat_alloc_vnode(mp, sb, sb->root_cluster, 0, 0x10);
    }
    else
    {
        // FAT12/16 root dir is special: fixed area
        root = fat_alloc_vnode(mp, sb, 0, sb->root_entries * 32, 0x10);
    }

    mp->root_vnode = root;
    return 0;
}

static int fat_unmount_fn(struct mount *mp)
{
    (void)mp;
    return 0;
}

static int fat_sync_fn(struct mount *mp)
{
    (void)mp;
    return 0;
}

// ---- directory / lookup ----

struct fat_dirent
{
    u8 name[11];
    u8 attr;
    u8 ntres;
    u8 crt_time_tenth;
    u16 crt_time;
    u16 crt_date;
    u16 lst_acc_date;
    u16 fst_clus_hi;
    u16 wrt_time;
    u16 wrt_date;
    u16 fst_clus_lo;
    u32 file_size;
};

static void fat_name_to_str(const u8 name[11], char *out)
{
    int i, j = 0;
    for (i = 0; i < 8 && name[i] != ' '; i++)
        out[j++] = name[i];
    if (name[8] != ' ')
    {
        out[j++] = '.';
        for (i = 8; i < 11 && name[i] != ' '; i++)
            out[j++] = name[i];
    }
    out[j] = 0;
}

static int fat_iter_dir(fat_super_t *sb, fat_node_t *node,
                        int (*cb)(struct fat_dirent *de, const char *name, void *ctx),
                        void *ctx)
{
    u8 *buf = (u8 *)malloc(sb->bytes_per_sector * sb->sectors_per_cluster);
    u32 cluster = node->first_cluster;
    u32 remaining = node->size;

    if (node->first_cluster == 0 && (sb->type == FAT_TYPE_12 || sb->type == FAT_TYPE_16))
    {
        // root dir in FAT12/16
        u32 sectors = (sb->root_entries * 32 + sb->bytes_per_sector - 1) / sb->bytes_per_sector;
        for (u32 s = 0; s < sectors; s++)
        {
            fat_read_sector(sb, sb->root_dir_start + s, buf);
            for (u32 off = 0; off < sb->bytes_per_sector; off += sizeof(struct fat_dirent))
            {
                struct fat_dirent *de = (struct fat_dirent *)(buf + off);
                if (de->name[0] == 0x00)
                    goto done;
                if (de->name[0] == 0xE5)
                    continue;
                if (de->attr == 0x0F)
                    continue; // LFN
                char name[13];
                fat_name_to_str(de->name, name);
                if (cb(de, name, ctx))
                    goto done;
            }
        }
        goto done;
    }

    while (cluster >= 2 && !sb->var->is_eoc(sb, cluster))
    {
        fat_read_cluster(sb, cluster, buf);
        for (u32 off = 0; off < sb->bytes_per_sector * sb->sectors_per_cluster; off += sizeof(struct fat_dirent))
        {
            struct fat_dirent *de = (struct fat_dirent *)(buf + off);
            if (de->name[0] == 0x00)
                goto done;
            if (de->name[0] == 0xE5)
                continue;
            if (de->attr == 0x0F)
                continue;
            char name[13];
            fat_name_to_str(de->name, name);
            if (cb(de, name, ctx))
                goto done;
        }
        // TODO: follow cluster chain via FAT
        break; // for now, single cluster
    }

done:
    free(buf);
    return 0;
}

struct lookup_ctx
{
    const char *want;
    struct vnode *dir;
    struct vnode **out;
};

static int fat_lookup_cb(struct fat_dirent *de, const char *name, void *ctxp)
{
    struct lookup_ctx *ctx = (struct lookup_ctx *)ctxp;
    if (strcmp(name, ctx->want) != 0)
        return 0;

    fat_node_t *dnode = (fat_node_t *)ctx->dir->fs_data;
    fat_super_t *sb = dnode->sb;

    u32 clus = ((u32)de->fst_clus_hi << 16) | de->fst_clus_lo;
    struct vnode *vn = fat_alloc_vnode(ctx->dir->mount, sb, clus, de->file_size, de->attr);
    *ctx->out = vn;
    return 1;
}

static int fat_lookup_vn(struct vnode *dir, const char *name, struct vnode **out)
{
    fat_node_t *node = (fat_node_t *)dir->fs_data;
    struct lookup_ctx ctx = {
        .want = name,
        .dir = dir,
        .out = out,
    };
    fat_iter_dir(node->sb, node, fat_lookup_cb, &ctx);
    return (*out) ? 0 : -1;
}

struct readdir_ctx
{
    usize want;
    usize cur;
    const char **name_out;
    vnode_type_t *type_out;
    int found;
};

static int fat_readdir_cb(struct fat_dirent *de, const char *name, void *ctxp)
{
    struct readdir_ctx *ctx = (struct readdir_ctx *)ctxp;
    if (ctx->cur == ctx->want)
    {
        static char name_buf[13];
        strcpy(name_buf, name);
        *ctx->name_out = name_buf;
        *ctx->type_out = (de->attr & 0x10) ? VNODE_TYPE_DIR : VNODE_TYPE_FILE;
        ctx->found = 1;
        return 1;
    }
    ctx->cur++;
    return 0;
}

static int fat_readdir_vn(struct vnode *vn, usize index, const char **name_out, vnode_type_t *type_out)
{
    fat_node_t *node = (fat_node_t *)vn->fs_data;
    struct readdir_ctx ctx = {
        .want = index,
        .cur = 0,
        .name_out = name_out,
        .type_out = type_out,
        .found = 0,
    };
    fat_iter_dir(node->sb, node, fat_readdir_cb, &ctx);
    return ctx.found ? 0 : -1;
}

// ---- read ----

static int fat_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out)
{
    fat_node_t *node = (fat_node_t *)vn->fs_data;
    fat_super_t *sb = node->sb;

    if (vn->type != VNODE_TYPE_FILE)
        return -1;

    if (off >= node->size)
    {
        *out = 0;
        return 0;
    }
    if (off + len > node->size)
        len = node->size - off;

    u8 *dst = (u8 *)buf;
    usize done = 0;

    u8 *cluster_buf = (u8 *)malloc(sb->bytes_per_sector * sb->sectors_per_cluster);
    u32 cluster = node->first_cluster;
    u32 cluster_size = sb->bytes_per_sector * sb->sectors_per_cluster;

    // TODO: follow cluster chain properly; for now assume contiguous
    while (done < len && cluster >= 2 && !sb->var->is_eoc(sb, cluster))
    {
        fat_read_cluster(sb, cluster, cluster_buf);

        u32 cluster_off = (off + done) % cluster_size;
        usize to_copy = cluster_size - cluster_off;
        if (to_copy > len - done)
            to_copy = len - done;

        memcpy(dst + done, cluster_buf + cluster_off, to_copy);
        done += to_copy;

        // TODO: next cluster via FAT
        break;
    }

    free(cluster_buf);
    *out = done;
    return 0;
}

// stubs for now
static int fat_write_vn(struct vnode *vn, const void *buf, usize off, usize len, usize *out)
{
    (void)vn;
    (void)buf;
    (void)off;
    (void)len;
    (void)out;
    return -1;
}

static int fat_create_vn(struct vnode *dir, const char *name, vnode_type_t type, struct vnode **out)
{
    (void)dir;
    (void)name;
    (void)type;
    (void)out;
    return -1;
}

static int fat_unlink_vn(struct vnode *dir, const char *name)
{
    (void)dir;
    (void)name;
    return -1;
}

static int fat_truncate_vn(struct vnode *vn, usize new_size)
{
    (void)vn;
    (void)new_size;
    return -1;
}

static int fat_getattr_vn(struct vnode *vn, void *stat_buf)
{
    fat_node_t *node = (fat_node_t *)vn->fs_data;
    if (!stat_buf)
        return -1;
    memcpy(stat_buf, node, sizeof(*node));
    return 0;
}

static int fat_setattr_vn(struct vnode *vn, const void *stat_buf)
{
    (void)vn;
    (void)stat_buf;
    return -1;
}
