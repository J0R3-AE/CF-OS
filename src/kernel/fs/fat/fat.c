#include "kernel/fs/fat.h"

#include "libc/string.h"
#include "libc/mem.h"
#include "libc/types.h"
#include "kernel/printf.h"
#include "libc/errno.h"

#include "kernel/mm/heap.h"

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
static void fat_destroy_vn(struct vnode *vn);
static u32 fat_cluster_to_lba(fat_super_t *sb, u32 cluster);

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
    .destroy = fat_destroy_vn,
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
    return blockdev_read(sb->bdev, lba, 1, buf);
}

static int fat_write_sector(fat_super_t *sb, u32 lba, const void *buf)
{
    return blockdev_write(sb->bdev, lba, 1, buf);
}

static int fat_write_cluster(fat_super_t *sb, u32 cluster, const void *buf)
{
    if (cluster < 2 || cluster >= sb->cluster_count + 2 || !buf)
        return ERR_IO_ERROR;
    u32 lba = fat_cluster_to_lba(sb, cluster);
    if (lba >= sb->volume_sectors ||
        sb->sectors_per_cluster > sb->volume_sectors - lba)
        return ERR_IO_ERROR;
    return blockdev_write(sb->bdev, lba, sb->sectors_per_cluster, buf);
}

static int fat_probe_type(fat_super_t *sb, u32 total_sectors,
                          fat_type_t *type_out)
{
    if (!type_out || !total_sectors || sb->bytes_per_sector != 512 ||
        sb->bdev->sector_size != sb->bytes_per_sector ||
        !sb->sectors_per_cluster ||
        (sb->sectors_per_cluster & (sb->sectors_per_cluster - 1)) != 0 ||
        sb->sectors_per_cluster > 128 ||
        !sb->reserved_sectors ||
        (sb->fat_count != 1 && sb->fat_count != 2))
        return ERR_NOT_SUPPORTED;

    sb->fat_sectors = sb->sectors_per_fat16 ?
                          sb->sectors_per_fat16 : sb->sectors_per_fat32;
    if (!sb->fat_sectors || sb->fat_sectors > total_sectors ||
        total_sectors > sb->bdev->sector_count)
        return ERR_IO_ERROR;

    sb->root_dir_sectors =
        ((u32)sb->root_entries * 32 + sb->bytes_per_sector - 1) /
        sb->bytes_per_sector;
    u32 fats_area = (u32)sb->fat_count * sb->fat_sectors;
    if (fats_area / sb->fat_count != sb->fat_sectors ||
        sb->reserved_sectors > total_sectors ||
        fats_area > total_sectors - sb->reserved_sectors ||
        sb->root_dir_sectors >
            total_sectors - sb->reserved_sectors - fats_area)
        return ERR_IO_ERROR;

    sb->fat_start = sb->reserved_sectors;
    sb->root_dir_start = sb->fat_start + fats_area;
    sb->data_start = sb->root_dir_start + sb->root_dir_sectors;
    sb->volume_sectors = total_sectors;
    sb->cluster_count =
        (total_sectors - (sb->data_start)) / sb->sectors_per_cluster;
    if (!sb->cluster_count)
        return ERR_IO_ERROR;

    if (sb->cluster_count < 4085)
        *type_out = FAT_TYPE_12;
    else if (sb->cluster_count < 65525)
        *type_out = FAT_TYPE_16;
    else
        *type_out = FAT_TYPE_32;

    if ((*type_out == FAT_TYPE_32 &&
         (sb->sectors_per_fat16 || sb->root_entries ||
          sb->root_cluster < 2 ||
          sb->root_cluster >= sb->cluster_count + 2)) ||
        (*type_out != FAT_TYPE_32 &&
         (!sb->sectors_per_fat16 || !sb->root_entries)))
        return ERR_IO_ERROR;

    if ((u64)sb->cluster_count + 2 >
        ((u64)sb->fat_sectors * sb->bytes_per_sector * 2) / 3 &&
        *type_out == FAT_TYPE_12)
        return ERR_IO_ERROR;
    if (*type_out == FAT_TYPE_16 &&
        (u64)sb->cluster_count + 2 >
            ((u64)sb->fat_sectors * (sb->bytes_per_sector / 2)))
        return ERR_IO_ERROR;
    if (*type_out == FAT_TYPE_32 &&
        (u64)sb->cluster_count + 2 >
            ((u64)sb->fat_sectors * (sb->bytes_per_sector / 4)))
        return ERR_IO_ERROR;

    return ERR_SUCCESS;
}

static struct vnode *fat_alloc_vnode(struct mount *mp, fat_super_t *sb, u32 first_cluster, u32 size, u8 attr)
{
    struct vnode *vn = (struct vnode *)malloc(sizeof(*vn));
    if (!vn)
        return NULL;
    memset(vn, 0, sizeof(*vn));
    vn->type = (attr & 0x10) ? VNODE_TYPE_DIR : VNODE_TYPE_FILE;
    vn->mount = mp;
    vn->ops = &fat_vnode_ops;
    vn->refcnt = 1;

    fat_node_t *node = (fat_node_t *)malloc(sizeof(*node));
    if (!node)
    {
        free(vn);
        return NULL;
    }
    node->sb = sb;
    node->first_cluster = first_cluster;
    node->size = size;
    node->attr = attr;
    node->dir_sector = 0;
    node->dir_offset = 0;
    node->has_dirent = 0;

    vn->fs_data = node;
    return vn;
}

static u32 fat_cluster_to_lba(fat_super_t *sb, u32 cluster)
{
    return sb->data_start + (cluster - 2) * sb->sectors_per_cluster;
}

static int fat_read_cluster(fat_super_t *sb, u32 cluster, void *buf)
{
    if (cluster < 2 || cluster >= sb->cluster_count + 2 ||
        !buf)
        return ERR_IO_ERROR;
    u32 lba = fat_cluster_to_lba(sb, cluster);
    if (lba >= sb->volume_sectors ||
        sb->sectors_per_cluster > sb->volume_sectors - lba)
        return ERR_IO_ERROR;
    return blockdev_read(sb->bdev, lba, sb->sectors_per_cluster, buf);
}

// ---- mount ----

static int fat_mount_fn(struct mount *mp, const char *opts)
{
    (void)opts;
    struct blockdev *bdev = blockdev_lookup(mp->source);
    if (!bdev)
        return ERR_DEVICE_NOT_FOUND;

    fat_super_t *sb = (fat_super_t *)malloc(sizeof(*sb));
    if (!sb)
        return ERR_OUT_OF_MEMORY;
    memset(sb, 0, sizeof(*sb));
    sb->bdev = bdev;

    u8 bpb[512];
    int result = blockdev_read(bdev, 0, 1, bpb);
    if (result != ERR_SUCCESS)
    {
        free(sb);
        return result;
    }
    if (bpb[510] != 0x55 || bpb[511] != 0xAA)
    {
        free(sb);
        return ERR_IO_ERROR;
    }

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
    sb->root_cluster &= 0x0FFFFFFF;

    u32 total_sectors = sb->total_sectors16 ?
                            sb->total_sectors16 : sb->total_sectors32;
    result = fat_probe_type(sb, total_sectors, &sb->type);
    if (result != ERR_SUCCESS)
    {
        free(sb);
        return result;
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
    default:
        free(sb);
        return ERR_NOT_SUPPORTED;
    }

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

    if (!root)
    {
        free(sb);
        return ERR_OUT_OF_MEMORY;
    }

    mp->fs_data = sb;
    mp->root_vnode = root;
    return ERR_SUCCESS;
}

static int fat_unmount_fn(struct mount *mp)
{
    if (!mp)
        return ERR_INVALID_ARGUMENT;
    if (mp->root_vnode && mp->root_vnode->refcnt > 1)
        return ERR_BUSY;

    if (mp->root_vnode)
        vnode_unref(mp->root_vnode);
    free(mp->fs_data);
    mp->fs_data = NULL;
    return ERR_SUCCESS;
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

static int fat_next_cluster(fat_super_t *sb, u32 cluster, u32 *next)
{
    u32 value;
    int result = sb->var->get_fat_entry(sb, cluster, &value);
    if (result != ERR_SUCCESS)
        return result;
    if (sb->var->is_eoc(sb, value))
    {
        *next = 0;
        return ERR_SUCCESS;
    }
    if (value < 2 || value >= sb->cluster_count + 2)
        return ERR_IO_ERROR;
    *next = value;
    return ERR_SUCCESS;
}

static int fat_iter_dir(fat_super_t *sb, fat_node_t *node,
                        int (*cb)(struct fat_dirent *de, const char *name,
                                  u32 sector, u16 offset, void *ctx),
                        void *ctx)
{
    u32 cluster_size = sb->bytes_per_sector * sb->sectors_per_cluster;
    u8 *buf = (u8 *)malloc(cluster_size);
    u32 cluster = node->first_cluster;
    if (!buf)
        return ERR_OUT_OF_MEMORY;

    if (node->first_cluster == 0 && (sb->type == FAT_TYPE_12 || sb->type == FAT_TYPE_16))
    {
        u32 sectors = sb->root_dir_sectors;
        u32 entries = 0;
        for (u32 s = 0; s < sectors; s++)
        {
            int result = fat_read_sector(sb, sb->root_dir_start + s, buf);
            if (result != ERR_SUCCESS)
            {
                free(buf);
                return result;
            }
            for (u32 off = 0; off < sb->bytes_per_sector; off += sizeof(struct fat_dirent))
            {
                if (entries++ >= sb->root_entries)
                {
                    free(buf);
                    return ERR_SUCCESS;
                }
                struct fat_dirent *de = (struct fat_dirent *)(buf + off);
                if (de->name[0] == 0x00)
                {
                    free(buf);
                    return ERR_SUCCESS;
                }
                if (de->name[0] == 0xE5)
                    continue;
                if (de->attr == 0x0F || (de->attr & 0x08))
                    continue;
                char name[13];
                fat_name_to_str(de->name, name);
                if (cb(de, name, sb->root_dir_start + s,
                       (u16)off, ctx))
                {
                    free(buf);
                    return ERR_SUCCESS;
                }
            }
        }
        free(buf);
        return ERR_SUCCESS;
    }

    for (u32 visited = 0; visited < sb->cluster_count; visited++)
    {
        int result = fat_read_cluster(sb, cluster, buf);
        if (result != ERR_SUCCESS)
        {
            free(buf);
            return result;
        }
        for (u32 off = 0; off < cluster_size; off += sizeof(struct fat_dirent))
        {
            struct fat_dirent *de = (struct fat_dirent *)(buf + off);
            if (de->name[0] == 0x00)
            {
                free(buf);
                return ERR_SUCCESS;
            }
            if (de->name[0] == 0xE5)
                continue;
            if (de->attr == 0x0F || (de->attr & 0x08))
                continue;
            char name[13];
            fat_name_to_str(de->name, name);
            u32 sector = fat_cluster_to_lba(sb, cluster) +
                         off / sb->bytes_per_sector;
            u16 sector_offset = (u16)(off % sb->bytes_per_sector);
            if (cb(de, name, sector, sector_offset, ctx))
            {
                free(buf);
                return ERR_SUCCESS;
            }
        }

        u32 next;
        result = fat_next_cluster(sb, cluster, &next);
        if (result != ERR_SUCCESS)
        {
            free(buf);
            return result;
        }
        if (!next)
        {
            free(buf);
            return ERR_SUCCESS;
        }
        cluster = next;
    }

    free(buf);
    return ERR_IO_ERROR;
}

struct lookup_ctx
{
    const char *want;
    struct vnode *dir;
    struct vnode **out;
    int error;
};

static int fat_lookup_cb(struct fat_dirent *de, const char *name,
                         u32 sector, u16 offset, void *ctxp)
{
    struct lookup_ctx *ctx = (struct lookup_ctx *)ctxp;
    if (strcasecmp(name, ctx->want) != 0)
        return 0;

    fat_node_t *dnode = (fat_node_t *)ctx->dir->fs_data;
    fat_super_t *sb = dnode->sb;

    u32 clus = ((u32)de->fst_clus_hi << 16) | de->fst_clus_lo;
    struct vnode *vn = fat_alloc_vnode(ctx->dir->mount, sb, clus, de->file_size, de->attr);
    if (!vn)
    {
        ctx->error = ERR_OUT_OF_MEMORY;
        return 1;
    }
    fat_node_t *node = (fat_node_t *)vn->fs_data;
    node->dir_sector = sector;
    node->dir_offset = offset;
    node->has_dirent = 1;
    *ctx->out = vn;
    return 1;
}

static int fat_lookup_vn(struct vnode *dir, const char *name, struct vnode **out)
{
    if (!dir || !name || !out || dir->type != VNODE_TYPE_DIR)
        return ERR_INVALID_ARGUMENT;
    *out = NULL;
    fat_node_t *node = (fat_node_t *)dir->fs_data;
    struct lookup_ctx ctx = {
        .want = name,
        .dir = dir,
        .out = out,
        .error = ERR_SUCCESS,
    };
    int result = fat_iter_dir(node->sb, node, fat_lookup_cb, &ctx);
    if (ctx.error != ERR_SUCCESS)
        return ctx.error;
    if (result != ERR_SUCCESS)
        return result;
    return (*out) ? ERR_SUCCESS : ERR_NOT_FOUND;
}

struct readdir_ctx
{
    usize want;
    usize cur;
    const char **name_out;
    vnode_type_t *type_out;
    int found;
};

static int fat_readdir_cb(struct fat_dirent *de, const char *name,
                          u32 sector, u16 offset, void *ctxp)
{
    (void)sector;
    (void)offset;
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
    if (!vn || !name_out || !type_out || vn->type != VNODE_TYPE_DIR)
        return ERR_INVALID_ARGUMENT;
    fat_node_t *node = (fat_node_t *)vn->fs_data;
    struct readdir_ctx ctx = {
        .want = index,
        .cur = 0,
        .name_out = name_out,
        .type_out = type_out,
        .found = 0,
    };
    int result = fat_iter_dir(node->sb, node, fat_readdir_cb, &ctx);
    if (result != ERR_SUCCESS)
        return result;
    return ctx.found ? ERR_SUCCESS : ERR_NOT_FOUND;
}

// ---- read ----

static int fat_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out)
{
    if (!vn || !buf || !out)
        return ERR_INVALID_ARGUMENT;
    fat_node_t *node = (fat_node_t *)vn->fs_data;
    fat_super_t *sb = node->sb;

    if (vn->type != VNODE_TYPE_FILE)
        return ERR_INVALID_ARGUMENT;

    if (off >= node->size || !len)
    {
        *out = 0;
        return ERR_SUCCESS;
    }
    if (len > node->size - off)
        len = node->size - off;

    u8 *dst = (u8 *)buf;
    usize done = 0;

    u8 *cluster_buf = (u8 *)malloc(sb->bytes_per_sector * sb->sectors_per_cluster);
    if (!cluster_buf)
        return ERR_OUT_OF_MEMORY;
    u32 cluster = node->first_cluster;
    u32 cluster_size = sb->bytes_per_sector * sb->sectors_per_cluster;
    u32 skip = (u32)(off / cluster_size);
    u32 cluster_off = (u32)(off % cluster_size);
    int result = ERR_SUCCESS;

    if (!cluster && node->size)
        result = ERR_IO_ERROR;
    for (u32 i = 0; result == ERR_SUCCESS && i < skip; i++)
    {
        result = fat_next_cluster(sb, cluster, &cluster);
        if (result == ERR_SUCCESS && !cluster)
            result = ERR_IO_ERROR;
    }

    for (u32 visited = 0; result == ERR_SUCCESS && done < len &&
                           visited <= sb->cluster_count; visited++)
    {
        result = fat_read_cluster(sb, cluster, cluster_buf);
        if (result != ERR_SUCCESS)
            break;
        usize to_copy = cluster_size - cluster_off;
        if (to_copy > len - done)
            to_copy = len - done;

        memcpy(dst + done, cluster_buf + cluster_off, to_copy);
        done += to_copy;
        cluster_off = 0;

        if (done < len)
        {
            result = fat_next_cluster(sb, cluster, &cluster);
            if (result == ERR_SUCCESS && !cluster)
                result = ERR_IO_ERROR;
        }
    }

    free(cluster_buf);
    if (result == ERR_SUCCESS && done != len)
        result = ERR_IO_ERROR;
    *out = done;
    return result;
}

static u32 fat_eoc_marker(fat_super_t *sb)
{
    if (sb->type == FAT_TYPE_12)
        return 0x0FFF;
    if (sb->type == FAT_TYPE_16)
        return 0xFFFF;
    return 0x0FFFFFFF;
}

static int fat_alloc_cluster(fat_super_t *sb, u32 *out)
{
    if (!sb || !out)
        return ERR_INVALID_ARGUMENT;

    u8 *zero = (u8 *)malloc(sb->bytes_per_sector * sb->sectors_per_cluster);
    if (!zero)
        return ERR_OUT_OF_MEMORY;
    memset(zero, 0, sb->bytes_per_sector * sb->sectors_per_cluster);

    for (u32 cluster = 2; cluster < sb->cluster_count + 2; cluster++)
    {
        u32 value;
        int result = sb->var->get_fat_entry(sb, cluster, &value);
        if (result != ERR_SUCCESS)
        {
            free(zero);
            return result;
        }
        if (value != 0)
            continue;

        result = fat_write_cluster(sb, cluster, zero);
        if (result == ERR_SUCCESS)
            result = sb->var->set_fat_entry(sb, cluster, fat_eoc_marker(sb));
        free(zero);
        if (result != ERR_SUCCESS)
            return result;
        *out = cluster;
        return ERR_SUCCESS;
    }

    free(zero);
    return ERR_NO_SPACE_LEFT;
}

static int fat_free_chain(fat_super_t *sb, u32 cluster)
{
    for (u32 visited = 0; cluster && visited < sb->cluster_count; visited++)
    {
        u32 next;
        int result = fat_next_cluster(sb, cluster, &next);
        if (result != ERR_SUCCESS)
            return result;
        result = sb->var->set_fat_entry(sb, cluster, 0);
        if (result != ERR_SUCCESS)
            return result;
        cluster = next;
    }
    return cluster ? ERR_IO_ERROR : ERR_SUCCESS;
}

static int fat_ensure_cluster(fat_node_t *node, u32 index, u32 *out)
{
    fat_super_t *sb = node->sb;
    u32 cluster = node->first_cluster;
    if (!cluster)
    {
        int result = fat_alloc_cluster(sb, &cluster);
        if (result != ERR_SUCCESS)
            return result;
        node->first_cluster = cluster;
    }

    for (u32 i = 0; i < index; i++)
    {
        u32 next;
        int result = fat_next_cluster(sb, cluster, &next);
        if (result != ERR_SUCCESS)
            return result;
        if (!next)
        {
            result = fat_alloc_cluster(sb, &next);
            if (result != ERR_SUCCESS)
                return result;
            result = sb->var->set_fat_entry(sb, cluster, next);
            if (result != ERR_SUCCESS)
            {
                fat_free_chain(sb, next);
                return result;
            }
        }
        cluster = next;
    }

    *out = cluster;
    return ERR_SUCCESS;
}

static int fat_update_dirent(fat_node_t *node)
{
    if (!node->has_dirent)
        return ERR_SUCCESS;
    if (node->dir_offset + sizeof(struct fat_dirent) >
        node->sb->bytes_per_sector)
        return ERR_IO_ERROR;

    u8 sector[512];
    int result = fat_read_sector(node->sb, node->dir_sector, sector);
    if (result != ERR_SUCCESS)
        return result;

    struct fat_dirent *de =
        (struct fat_dirent *)(sector + node->dir_offset);
    de->fst_clus_hi = (u16)(node->first_cluster >> 16);
    de->fst_clus_lo = (u16)node->first_cluster;
    de->file_size = node->size;
    return fat_write_sector(node->sb, node->dir_sector, sector);
}

static int fat_find_free_slot(fat_super_t *sb, fat_node_t *dir,
                              u32 *sector_out, u16 *offset_out)
{
    u8 sector_buf[512];
    if (!dir->first_cluster &&
        (sb->type == FAT_TYPE_12 || sb->type == FAT_TYPE_16))
    {
        u32 entries = 0;
        for (u32 s = 0; s < sb->root_dir_sectors; s++)
        {
            u32 lba = sb->root_dir_start + s;
            int result = fat_read_sector(sb, lba, sector_buf);
            if (result != ERR_SUCCESS)
                return result;
            for (u32 offset = 0; offset + 32 <= sb->bytes_per_sector;
                 offset += 32)
            {
                if (entries++ >= sb->root_entries)
                    return ERR_NO_SPACE_LEFT;
                struct fat_dirent *de =
                    (struct fat_dirent *)(sector_buf + offset);
                if (de->name[0] == 0 || de->name[0] == 0xE5)
                {
                    *sector_out = lba;
                    *offset_out = (u16)offset;
                    return ERR_SUCCESS;
                }
            }
        }
        return ERR_NO_SPACE_LEFT;
    }

    u32 cluster = dir->first_cluster;
    if (!cluster)
        return ERR_IO_ERROR;
    u32 last_cluster = cluster;
    for (u32 visited = 0; visited < sb->cluster_count; visited++)
    {
        u32 first_lba = fat_cluster_to_lba(sb, cluster);
        for (u32 s = 0; s < sb->sectors_per_cluster; s++)
        {
            int result = fat_read_sector(sb, first_lba + s, sector_buf);
            if (result != ERR_SUCCESS)
                return result;
            for (u32 offset = 0; offset + 32 <= sb->bytes_per_sector;
                 offset += 32)
            {
                struct fat_dirent *de =
                    (struct fat_dirent *)(sector_buf + offset);
                if (de->name[0] == 0 || de->name[0] == 0xE5)
                {
                    *sector_out = first_lba + s;
                    *offset_out = (u16)offset;
                    return ERR_SUCCESS;
                }
            }
        }

        u32 next;
        int result = fat_next_cluster(sb, cluster, &next);
        if (result != ERR_SUCCESS)
            return result;
        if (!next)
        {
            u32 new_cluster;
            result = fat_alloc_cluster(sb, &new_cluster);
            if (result != ERR_SUCCESS)
                return result;
            result = sb->var->set_fat_entry(sb, cluster, new_cluster);
            if (result != ERR_SUCCESS)
            {
                fat_free_chain(sb, new_cluster);
                return result;
            }
            *sector_out = fat_cluster_to_lba(sb, new_cluster);
            *offset_out = 0;
            return ERR_SUCCESS;
        }
        last_cluster = cluster = next;
    }
    (void)last_cluster;
    return ERR_IO_ERROR;
}

static int fat_make_short_name(const char *name, u8 out[11])
{
    if (!name || !name[0])
        return ERR_INVALID_ARGUMENT;

    memset(out, ' ', 11);
    u32 base_len = 0;
    u32 ext_len = 0;
    int in_ext = 0;
    for (u32 i = 0; name[i]; i++)
    {
        unsigned char c = (unsigned char)name[i];
        if (c == '.')
        {
            if (in_ext || base_len == 0)
                return ERR_INVALID_ARGUMENT;
            in_ext = 1;
            continue;
        }
        if (c <= 0x20 || c >= 0x7F ||
            c == '/' || c == '\\' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            return ERR_INVALID_ARGUMENT;
        if (c >= 'a' && c <= 'z')
            c = (unsigned char)(c - 'a' + 'A');
        if (in_ext)
        {
            if (++ext_len > 3)
                return ERR_NAME_TOO_LONG;
            out[8 + ext_len - 1] = c;
        }
        else
        {
            if (++base_len > 8)
                return ERR_NAME_TOO_LONG;
            out[base_len - 1] = c;
        }
    }
    return ERR_SUCCESS;
}

static int fat_write_vn(struct vnode *vn, const void *buf, usize off, usize len, usize *out)
{
    if (!vn || !buf || !out || vn->type != VNODE_TYPE_FILE)
        return ERR_INVALID_ARGUMENT;
    *out = 0;
    if (!len)
        return ERR_SUCCESS;
    if (off > 0xFFFFFFFFu || len > 0xFFFFFFFFu - off)
        return ERR_NO_SPACE_LEFT;

    fat_node_t *node = (fat_node_t *)vn->fs_data;
    usize end = off + len;
    if (end > node->size)
    {
        int result = fat_truncate_vn(vn, end);
        if (result != ERR_SUCCESS)
            return result;
    }

    fat_super_t *sb = node->sb;
    u32 cluster_size = sb->bytes_per_sector * sb->sectors_per_cluster;
    u8 *cluster_buf = (u8 *)malloc(cluster_size);
    if (!cluster_buf)
        return ERR_OUT_OF_MEMORY;

    usize done = 0;
    int result = ERR_SUCCESS;
    while (done < len)
    {
        u32 cluster;
        u32 file_offset = (u32)(off + done);
        result = fat_ensure_cluster(node, file_offset / cluster_size, &cluster);
        if (result != ERR_SUCCESS)
            break;
        result = fat_read_cluster(sb, cluster, cluster_buf);
        if (result != ERR_SUCCESS)
            break;

        u32 cluster_offset = file_offset % cluster_size;
        usize amount = cluster_size - cluster_offset;
        if (amount > len - done)
            amount = len - done;
        memcpy(cluster_buf + cluster_offset, (const u8 *)buf + done, amount);
        result = fat_write_cluster(sb, cluster, cluster_buf);
        if (result != ERR_SUCCESS)
            break;
        done += amount;
    }

    free(cluster_buf);
    if (node->size < off + done)
        node->size = (u32)(off + done);
    int metadata_result = fat_update_dirent(node);
    *out = done;
    return result != ERR_SUCCESS ? result : metadata_result;
}

static int fat_create_vn(struct vnode *dir, const char *name, vnode_type_t type, struct vnode **out)
{
    if (!dir || !name || !out || dir->type != VNODE_TYPE_DIR ||
        (type != VNODE_TYPE_FILE && type != VNODE_TYPE_DIR))
        return ERR_INVALID_ARGUMENT;
    *out = NULL;

    u8 short_name[11];
    int result = fat_make_short_name(name, short_name);
    if (result != ERR_SUCCESS)
        return result;

    struct vnode *existing = NULL;
    result = fat_lookup_vn(dir, name, &existing);
    if (result == ERR_SUCCESS)
    {
        vnode_unref(existing);
        return ERR_ALREADY_EXISTS;
    }
    if (result != ERR_NOT_FOUND)
        return result;

    fat_node_t *dir_node = (fat_node_t *)dir->fs_data;
    fat_super_t *sb = dir_node->sb;
    u32 sector_lba;
    u16 offset;
    result = fat_find_free_slot(sb, dir_node, &sector_lba, &offset);
    if (result != ERR_SUCCESS)
        return result;

    u32 first_cluster = 0;
    if (type == VNODE_TYPE_DIR)
    {
        result = fat_alloc_cluster(sb, &first_cluster);
        if (result != ERR_SUCCESS)
            return result;
    }
    struct vnode *vn = fat_alloc_vnode(dir->mount, sb, first_cluster, 0,
                                       type == VNODE_TYPE_DIR ? 0x10 : 0x20);
    if (!vn)
    {
        if (first_cluster)
            fat_free_chain(sb, first_cluster);
        return ERR_OUT_OF_MEMORY;
    }
    fat_node_t *node = (fat_node_t *)vn->fs_data;
    node->dir_sector = sector_lba;
    node->dir_offset = offset;
    node->has_dirent = 1;

    if (type == VNODE_TYPE_DIR)
    {
        u32 cluster_size = sb->bytes_per_sector * sb->sectors_per_cluster;
        u8 *contents = (u8 *)malloc(cluster_size);
        if (!contents)
        {
            result = ERR_OUT_OF_MEMORY;
            goto fail;
        }
        memset(contents, 0, cluster_size);
        struct fat_dirent *dot = (struct fat_dirent *)contents;
        memset(dot->name, ' ', 11);
        dot->name[0] = '.';
        dot->attr = 0x10;
        dot->fst_clus_hi = (u16)(first_cluster >> 16);
        dot->fst_clus_lo = (u16)first_cluster;
        struct fat_dirent *dotdot = dot + 1;
        memset(dotdot->name, ' ', 11);
        dotdot->name[0] = '.';
        dotdot->name[1] = '.';
        dotdot->attr = 0x10;
        dotdot->fst_clus_hi = (u16)(dir_node->first_cluster >> 16);
        dotdot->fst_clus_lo = (u16)dir_node->first_cluster;
        result = fat_write_cluster(sb, first_cluster, contents);
        free(contents);
        if (result != ERR_SUCCESS)
            goto fail;
    }

    u8 sector[512];
    result = fat_read_sector(sb, sector_lba, sector);
    if (result != ERR_SUCCESS)
        goto fail;
    struct fat_dirent *de = (struct fat_dirent *)(sector + offset);
    memset(de, 0, sizeof(*de));
    memcpy(de->name, short_name, sizeof(short_name));
    de->attr = type == VNODE_TYPE_DIR ? 0x10 : 0x20;
    de->fst_clus_hi = (u16)(first_cluster >> 16);
    de->fst_clus_lo = (u16)first_cluster;
    result = fat_write_sector(sb, sector_lba, sector);
    if (result != ERR_SUCCESS)
        goto fail;

    *out = vn;
    return ERR_SUCCESS;

fail:
    vnode_unref(vn);
    if (first_cluster)
        fat_free_chain(sb, first_cluster);
    return result;
}

struct fat_empty_ctx
{
    int nonempty;
};

static int fat_empty_cb(struct fat_dirent *de, const char *name,
                        u32 sector, u16 offset, void *ctxp)
{
    (void)de;
    (void)sector;
    (void)offset;
    struct fat_empty_ctx *ctx = (struct fat_empty_ctx *)ctxp;
    if (strcmp(name, ".") && strcmp(name, ".."))
    {
        ctx->nonempty = 1;
        return 1;
    }
    return 0;
}

static int fat_unlink_vn(struct vnode *dir, const char *name)
{
    if (!dir || !name || dir->type != VNODE_TYPE_DIR)
        return ERR_INVALID_ARGUMENT;
    struct vnode *vn = NULL;
    int result = fat_lookup_vn(dir, name, &vn);
    if (result != ERR_SUCCESS)
        return result;

    fat_node_t *node = (fat_node_t *)vn->fs_data;
    if (vn->type == VNODE_TYPE_DIR)
    {
        struct fat_empty_ctx ctx = {0};
        result = fat_iter_dir(node->sb, node, fat_empty_cb, &ctx);
        if (result == ERR_SUCCESS && ctx.nonempty)
            result = ERR_DIRECTORY_NOT_EMPTY;
    }
    if (result == ERR_SUCCESS)
    {
        u8 sector[512];
        result = fat_read_sector(node->sb, node->dir_sector, sector);
        if (result == ERR_SUCCESS)
        {
            sector[node->dir_offset] = 0xE5;
            result = fat_write_sector(node->sb, node->dir_sector, sector);
        }
    }
    if (result == ERR_SUCCESS && node->first_cluster)
        result = fat_free_chain(node->sb, node->first_cluster);
    vnode_unref(vn);
    return result;
}

static int fat_truncate_vn(struct vnode *vn, usize new_size)
{
    if (!vn || vn->type != VNODE_TYPE_FILE || new_size > 0xFFFFFFFFu)
        return ERR_INVALID_ARGUMENT;
    fat_node_t *node = (fat_node_t *)vn->fs_data;
    fat_super_t *sb = node->sb;
    u32 cluster_size = sb->bytes_per_sector * sb->sectors_per_cluster;
    u32 old_size = node->size;

    if (new_size < old_size)
    {
        u32 keep_count = new_size ?
            ((u32)new_size + cluster_size - 1) / cluster_size : 0;
        if (!keep_count)
        {
            int result = fat_free_chain(sb, node->first_cluster);
            if (result != ERR_SUCCESS)
                return result;
            node->first_cluster = 0;
        }
        else
        {
            u32 last;
            int result = fat_ensure_cluster(node, keep_count - 1, &last);
            if (result != ERR_SUCCESS)
                return result;
            u32 tail;
            result = fat_next_cluster(sb, last, &tail);
            if (result != ERR_SUCCESS)
                return result;
            result = sb->var->set_fat_entry(sb, last, fat_eoc_marker(sb));
            if (result != ERR_SUCCESS)
                return result;
            result = fat_free_chain(sb, tail);
            if (result != ERR_SUCCESS)
                return result;
        }
    }
    else if (new_size > old_size)
    {
        if (new_size)
        {
            u32 last;
            int result = fat_ensure_cluster(node,
                ((u32)new_size - 1) / cluster_size, &last);
            if (result != ERR_SUCCESS)
                return result;
        }

        u8 *zero = (u8 *)malloc(cluster_size);
        if (!zero)
            return ERR_OUT_OF_MEMORY;
        memset(zero, 0, cluster_size);
        u32 cursor = old_size;
        int result = ERR_SUCCESS;
        while (cursor < new_size)
        {
            u32 cluster;
            result = fat_ensure_cluster(node, cursor / cluster_size, &cluster);
            if (result != ERR_SUCCESS)
                break;
            u32 cluster_offset = cursor % cluster_size;
            u32 amount = cluster_size - cluster_offset;
            if (amount > new_size - cursor)
                amount = (u32)new_size - cursor;
            if (cluster_offset == 0 && amount == cluster_size)
            {
                result = fat_write_cluster(sb, cluster, zero);
            }
            else
            {
                u8 *data = (u8 *)malloc(cluster_size);
                if (!data)
                {
                    result = ERR_OUT_OF_MEMORY;
                    break;
                }
                result = fat_read_cluster(sb, cluster, data);
                if (result == ERR_SUCCESS)
                {
                    memset(data + cluster_offset, 0, amount);
                    result = fat_write_cluster(sb, cluster, data);
                }
                free(data);
            }
            if (result != ERR_SUCCESS)
                break;
            cursor += amount;
        }
        free(zero);
        if (result != ERR_SUCCESS)
            return result;
    }

    node->size = (u32)new_size;
    return fat_update_dirent(node);
}

static int fat_getattr_vn(struct vnode *vn, void *stat_buf)
{
    fat_node_t *node = (fat_node_t *)vn->fs_data;
    if (!stat_buf)
        return ERR_INVALID_ARGUMENT;
    vfs_stat_t *stat = (vfs_stat_t *)stat_buf;
    memset(stat, 0, sizeof(*stat));
    stat->type = vn->type;
    stat->size = node->size;
    stat->capacity = node->size;
    stat->mode = node->attr;
    return ERR_SUCCESS;
}

static int fat_setattr_vn(struct vnode *vn, const void *stat_buf)
{
    (void)vn;
    (void)stat_buf;
    return ERR_READ_ONLY_FS;
}

static void fat_destroy_vn(struct vnode *vn)
{
    if (vn)
        free(vn->fs_data);
}
