#include "fs/ext2.h"
#include "libk/string.h"
#include "libk/mem.h"
#include "libk/math.h"
#include "libk/printf.h"
#include "libk/log.h"

#include "mm/heap.h"

extern struct blockdev *blockdev_open(const char *name);
// --------------------------------

static int ext2_mount_fn(struct mount *mp, const char *opts);
static int ext2_unmount_fn(struct mount *mp);
static int ext2_sync_fn(struct mount *mp);

static int ext2_lookup_vn(struct vnode *dir, const char *name, struct vnode **out);
static int ext2_create_vn(struct vnode *dir, const char *name, vnode_type_t type, struct vnode **out);
static int ext2_unlink_vn(struct vnode *dir, const char *name);
static int ext2_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out);
static int ext2_write_vn(struct vnode *vn, const void *buf, usize off, usize len, usize *out);
static int ext2_readdir_vn(struct vnode *vn, usize index, const char **name_out, vnode_type_t *type_out);
static int ext2_truncate_vn(struct vnode *vn, usize new_size);
static int ext2_getattr_vn(struct vnode *vn, void *stat_buf);
static int ext2_setattr_vn(struct vnode *vn, const void *stat_buf);

static int ext2_alloc_block(ext2_super_t *sb, u32 *out_block);
static int ext2_alloc_inode(ext2_super_t *sb, u32 *out_ino, u16 mode);
static int ext2_dir_add_entry(ext2_super_t *sb,
                              ext2_inode_disk_t *dir_inode,
                              u32 child_ino,
                              const char *name,
                              u8 file_type);

static const struct fs_ops ext2_fs_ops = {
    .mount = ext2_mount_fn,
    .unmount = ext2_unmount_fn,
    .sync = ext2_sync_fn,
};

static const struct vnode_ops ext2_vnode_ops = {
    .lookup = ext2_lookup_vn,
    .create = ext2_create_vn,
    .unlink = ext2_unlink_vn,
    .read = ext2_read_vn,
    .write = ext2_write_vn,
    .readdir = ext2_readdir_vn,
    .truncate = ext2_truncate_vn,
    .getattr = ext2_getattr_vn,
    .setattr = ext2_setattr_vn,
};

static struct fs_type ext2_type = {
    .name = "ext2",
    .fs_ops = &ext2_fs_ops,
    .root_vnode = NULL
};

void ext2_init(void)
{
    vfs_register_fs(&ext2_type);
}

// ---- helpers ----

static int ext2_read_block(ext2_super_t *sb, u32 block, void *buf)
{
    u32 lba = (sb->block_size / sb->bdev->sector_size) * block;
    u32 count = sb->block_size / sb->bdev->sector_size;
    return sb->bdev->read(sb->bdev, lba, count, buf);
}

static int ext2_write_block(ext2_super_t *sb, u32 block, const void *buf)
{
    u32 lba = (sb->block_size / sb->bdev->sector_size) * block;
    u32 count = sb->block_size / sb->bdev->sector_size;
    return sb->bdev->write(sb->bdev, lba, count, buf);
}

static int ext2_read_inode(ext2_super_t *sb, u32 ino, ext2_inode_disk_t *out)
{
    u32 inodes_per_group = sb->inodes_per_group;
    u32 group = (ino - 1) / inodes_per_group;
    u32 index = (ino - 1) % inodes_per_group;

    u32 bgdt_block = sb->bgdt_block;
    ext2_bg_desc_t bgd;
    u8 buf[4096];

    ext2_read_block(sb, bgdt_block + (group * sizeof(ext2_bg_desc_t)) / sb->block_size, buf);
    memcpy(&bgd, buf + (group % (sb->block_size / sizeof(ext2_bg_desc_t))) * sizeof(ext2_bg_desc_t), sizeof(bgd));

    u32 inode_table_block = bgd.inode_table;
    u32 inode_size = sb->inode_size;
    u32 inodes_per_block = sb->block_size / inode_size;
    u32 block = inode_table_block + (index / inodes_per_block);
    u32 offset = (index % inodes_per_block) * inode_size;

    ext2_read_block(sb, block, buf);
    memcpy(out, buf + offset, sizeof(ext2_inode_disk_t));
    return 0;
}

static int ext2_write_inode(ext2_super_t *sb, u32 ino, const ext2_inode_disk_t *in)
{
    u32 inodes_per_group = sb->inodes_per_group;
    u32 group = (ino - 1) / inodes_per_group;
    u32 index = (ino - 1) % inodes_per_group;

    u32 bgdt_block = sb->bgdt_block;
    ext2_bg_desc_t bgd;
    u8 buf[4096];

    /* load BGDT entry */
    ext2_read_block(sb,
                    bgdt_block + (group * sizeof(ext2_bg_desc_t)) / sb->block_size,
                    buf);
    memcpy(&bgd,
           buf + (group % (sb->block_size / sizeof(ext2_bg_desc_t))) * sizeof(ext2_bg_desc_t),
           sizeof(bgd));

    u32 inode_table_block = bgd.inode_table;
    u32 inode_size = sb->inode_size;
    u32 inodes_per_block = sb->block_size / inode_size;
    u32 block = inode_table_block + (index / inodes_per_block);
    u32 offset = (index % inodes_per_block) * inode_size;

    ext2_read_block(sb, block, buf);
    memcpy(buf + offset, in, sizeof(ext2_inode_disk_t));

    return ext2_write_block(sb, block, buf);
}

static int ext2_alloc_inode(ext2_super_t *sb, u32 *out_ino, u16 mode)
{
    u32 inodes_per_group = sb->inodes_per_group;
    u32 groups = (sb->inodes_count + inodes_per_group - 1) / inodes_per_group;

    u8 *bitmap = malloc(sb->block_size);
    if (!bitmap)
        return -1;

    for (u32 group = 0; group < groups; group++)
    {
        /* Load BGDT entry */
        u32 bgdt_block = sb->bgdt_block;
        u32 desc_per_block = sb->block_size / sizeof(ext2_bg_desc_t);

        u8 buf[4096];
        if (ext2_read_block(sb,
                            bgdt_block + (group / desc_per_block),
                            buf) != 0)
            continue;

        ext2_bg_desc_t *bgd =
            (ext2_bg_desc_t *)(buf + (group % desc_per_block) * sizeof(ext2_bg_desc_t));

        if (bgd->free_inodes_count == 0)

            continue;

        /* Read inode bitmap */
        if (ext2_read_block(sb, bgd->inode_bitmap, bitmap) != 0)
        {
            continue;

            /* Scan bitmap */
            for (u32 byte = 0; byte < sb->block_size; byte++)
            {
                if (bitmap[byte] != 0xFF)
                {
                    for (u32 bit = 0; bit < 8; bit++)
                    {
                        if (!(bitmap[byte] & (1 << bit)))
                        {
                            /* Mark inode used */
                            bitmap[byte] |= (1 << bit);
                            ext2_write_block(sb, bgd->inode_bitmap, bitmap);

                            /* Compute absolute inode number (1‑based) */
                            u32 local_index = byte * 8 + bit;
                            u32 ino = group * inodes_per_group + local_index + 1;

                            /* Initialize on‑disk inode */
                            ext2_inode_disk_t inode;
                            memset(&inode, 0, sizeof(inode));
                            inode.mode = mode;
                            inode.size = 0;
                            inode.blocks = 0;
                            inode.links_count = 1;

                            /* Write inode to disk */
                            ext2_write_inode(sb, ino, &inode);

                            /* Update counters */
                            bgd->free_inodes_count--;
                            sb->free_inodes_count--;

                            memcpy(buf + (group % desc_per_block) * sizeof(ext2_bg_desc_t),
                                   bgd, sizeof(ext2_bg_desc_t));

                            ext2_write_block(sb,
                                             bgdt_block + (group / desc_per_block),
                                             buf);

                            free(bitmap);
                            *out_ino = ino;
                            return 0;
                        }
                    }
                }
            }
        }

        free(bitmap);
        return -1; /* no free inode found */
    }
}

static struct vnode *ext2_alloc_vnode(struct mount *mp, ext2_super_t *sb, u32 ino, ext2_inode_disk_t *disk)
{
    struct vnode *vn = (struct vnode *)malloc(sizeof(*vn));
    memset(vn, 0, sizeof(*vn));
    vn->type = (disk->mode & 0x4000) ? VNODE_TYPE_DIR : VNODE_TYPE_FILE;
    vn->mount = mp;
    vn->ops = &ext2_vnode_ops;
    vn->refcnt = 1;

    ext2_node_t *node = (ext2_node_t *)malloc(sizeof(*node));
    node->sb = sb;
    node->ino = ino;
    memcpy(&node->inode, disk, sizeof(*disk));

    vn->fs_data = node;
    return vn;
}

// ---- mount ----

static int ext2_mount_fn(struct mount *mp, const char *opts)
{
    struct blockdev *bdev = blockdev_open(opts);
    if (!bdev)
    {
        return -1;
    }

    ext2_super_t *sb = (ext2_super_t *)malloc(sizeof(*sb));
    memset(sb, 0, sizeof(*sb));
    sb->bdev = bdev;

    u8 buf[1024];
    // superblock at block 1, offset 1024
    int rc = bdev->read(bdev, 2, 2, buf); // assuming 512-byte sectors

    memcpy(&sb->inodes_count, buf + 0, 4);
    memcpy(&sb->blocks_count, buf + 4, 4);
    memcpy(&sb->r_blocks_count, buf + 8, 4);
    memcpy(&sb->free_blocks_count, buf + 12, 4);
    memcpy(&sb->free_inodes_count, buf + 16, 4);
    memcpy(&sb->first_data_block, buf + 20, 4);
    memcpy(&sb->log_block_size, buf + 24, 4);
    memcpy(&sb->blocks_per_group, buf + 32, 4);
    memcpy(&sb->inodes_per_group, buf + 40, 4);
    memcpy(&sb->magic, buf + 56, 2);
    memcpy(&sb->inode_size, buf + 88, 2);
    memcpy(&sb->first_ino, buf + 84, 4);

    if (sb->magic != 0xEF53)
    {
        free(sb);
        return -1;
    }

    sb->block_size = 1024U << sb->log_block_size;
    sb->groups_count = (sb->blocks_count + sb->blocks_per_group - 1) / sb->blocks_per_group;
    sb->bgdt_block = (sb->block_size == 1024) ? 2 : 1;

    mp->fs_data = sb;

    // root inode
    ext2_inode_disk_t root_inode;
    ext2_read_inode(sb, 2, &root_inode);
    struct vnode *root = ext2_alloc_vnode(mp, sb, 2, &root_inode);
    mp->root_vnode = root;

    return 0;
}
static int ext2_unmount_fn(struct mount *mp)
{
    // TODO: free all vnodes, sb, etc.
    return 0;
}

static int ext2_sync_fn(struct mount *mp)
{
    // TODO: flush dirty inodes/blocks
    return 0;
}

// ---- vnode ops (minimal, mostly read-only) ----

struct ext2_dirent
{
    u32 ino;
    u16 rec_len;
    u8 name_len;
    u8 file_type;
    char name[];
};

static int ext2_read_block_for_inode(ext2_super_t *sb, ext2_inode_disk_t *inode, u32 iblock, void *buf)
{
    if (iblock < 12)
    {
        if (inode->block[iblock] == 0)
            return -1;
        return ext2_read_block(sb, inode->block[iblock], buf);
    }
    // TODO: indirect blocks
    return -1;
}

static int ext2_lookup_vn(struct vnode *dir, const char *name, struct vnode **out)
{
    ext2_node_t *node = (ext2_node_t *)dir->fs_data;
    ext2_super_t *sb = node->sb;
    ext2_inode_disk_t *inode = &node->inode;

    if (!(inode->mode & 0x4000))
        return -1; // not dir

    u8 *block = (u8 *)malloc(sb->block_size);
    u32 size = inode->size;
    u32 off = 0;

    while (off < size)
    {
        u32 blk_index = off / sb->block_size;
        u32 blk_off = off % sb->block_size;

        if (ext2_read_block_for_inode(sb, inode, blk_index, block) != 0)
            break;

        u32 pos = blk_off;
        while (pos < sb->block_size && off < size)
        {
            struct ext2_dirent *de = (struct ext2_dirent *)(block + pos);
            if (de->ino != 0 && de->name_len > 0)
            {
                char tmp[256];
                memcpy(tmp, de->name, de->name_len);
                tmp[de->name_len] = 0;
                if (strcmp(tmp, name) == 0)
                {
                    ext2_inode_disk_t child_inode;
                    ext2_read_inode(sb, de->ino, &child_inode);
                    *out = ext2_alloc_vnode(dir->mount, sb, de->ino, &child_inode);
                    free(block);
                    return 0;
                }
            }
            pos += de->rec_len;
            off += de->rec_len;
        }
    }

    free(block);
    return -1;
}

static int ext2_readdir_vn(struct vnode *vn, usize index, const char **name_out, vnode_type_t *type_out)
{
    ext2_node_t *node = (ext2_node_t *)vn->fs_data;
    ext2_super_t *sb = node->sb;
    ext2_inode_disk_t *inode = &node->inode;

    if (!(inode->mode & 0x4000))
        return -1; // not dir

    u8 *block = (u8 *)malloc(sb->block_size);
    u32 size = inode->size;
    u32 off = 0;
    usize cur = 0;

    static char name_buf[256];

    while (off < size)
    {
        u32 blk_index = off / sb->block_size;
        u32 blk_off = off % sb->block_size;

        if (ext2_read_block_for_inode(sb, inode, blk_index, block) != 0)
            break;

        u32 pos = blk_off;
        while (pos < sb->block_size && off < size)
        {
            struct ext2_dirent *de = (struct ext2_dirent *)(block + pos);
            if (de->ino != 0 && de->name_len > 0)
            {
                if (cur == index)
                {
                    memcpy(name_buf, de->name, de->name_len);
                    name_buf[de->name_len] = 0;
                    *name_out = name_buf;
                    *type_out = (de->file_type == 2) ? VNODE_TYPE_DIR : VNODE_TYPE_FILE;
                    free(block);
                    return 0;
                }
                cur++;
            }
            pos += de->rec_len;
            off += de->rec_len;
        }
    }

    free(block);
    return -1;
}

static int ext2_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out)
{
    ext2_node_t *node = (ext2_node_t *)vn->fs_data;
    ext2_super_t *sb = node->sb;
    ext2_inode_disk_t *inode = &node->inode;

    if (vn->type != VNODE_TYPE_FILE)
        return -1;

    if (off >= inode->size)
    {
        *out = 0;
        return 0;
    }

    if (off + len > inode->size)
        len = inode->size - off;

    u8 *dst = (u8 *)buf;
    usize done = 0;

    u8 *block = (u8 *)malloc(sb->block_size);

    while (done < len)
    {
        u32 blk_index = (off + done) / sb->block_size;
        u32 blk_off = (off + done) % sb->block_size;
        usize to_copy = sb->block_size - blk_off;
        if (to_copy > len - done)
            to_copy = len - done;

        if (ext2_read_block_for_inode(sb, inode, blk_index, block) != 0)
            break;

        memcpy(dst + done, block + blk_off, to_copy);
        done += to_copy;
    }

    free(block);
    *out = done;
    return 0;
}

static int ext2_alloc_block(ext2_super_t *sb, u32 *out_block)
{
    u32 blocks_per_group = sb->blocks_per_group;
    u32 groups = sb->groups_count;

    u8 *bitmap = malloc(sb->block_size);
    if (!bitmap)
        return -1;

    for (u32 group = 0; group < groups; group++)
    {
        /* Load BGDT entry */
        u32 bgdt_block = sb->bgdt_block;
        u32 desc_per_block = sb->block_size / sizeof(ext2_bg_desc_t);

        u8 buf[4096];
        if (ext2_read_block(sb, bgdt_block + (group / desc_per_block), buf) != 0)
            continue;

        ext2_bg_desc_t *bgd = (ext2_bg_desc_t *)(buf + (group % desc_per_block) * sizeof(ext2_bg_desc_t));

        if (bgd->free_blocks_count == 0)
            continue; // no free blocks here

        /* Read block bitmap */
        if (ext2_read_block(sb, bgd->block_bitmap, bitmap) != 0)
            continue;

        /* Scan for first zero bit */
        for (u32 byte = 0; byte < sb->block_size; byte++)
        {
            if (bitmap[byte] != 0xFF)
            {
                for (u32 bit = 0; bit < 8; bit++)
                {
                    if (!(bitmap[byte] & (1 << bit)))
                    {
                        /* Found free block */
                        bitmap[byte] |= (1 << bit);

                        /* Write bitmap back */
                        sb->bdev->write(sb->bdev,
                                        (sb->block_size / sb->bdev->sector_size) * bgd->block_bitmap,
                                        sb->block_size / sb->bdev->sector_size,
                                        bitmap);

                        /* Compute absolute block number */
                        u32 block = group * blocks_per_group + byte * 8 + bit;

                        /* Update counters */
                        bgd->free_blocks_count--;
                        sb->free_blocks_count--;

                        /* Write updated BGDT entry */
                        memcpy(buf + (group % desc_per_block) * sizeof(ext2_bg_desc_t),
                               bgd, sizeof(ext2_bg_desc_t));

                        sb->bdev->write(sb->bdev,
                                        (sb->block_size / sb->bdev->sector_size) * (bgdt_block + (group / desc_per_block)),
                                        sb->block_size / sb->bdev->sector_size,
                                        buf);

                        free(bitmap);
                        *out_block = block;
                        return 0;
                    }
                }
            }
        }
    }

    free(bitmap);
    return -1; // no free blocks anywhere
}

static int ext2_dir_add_entry(ext2_super_t *sb,
                              ext2_inode_disk_t *dir_inode,
                              u32 child_ino,
                              const char *name,
                              u8 file_type)
{
    u32 block_size = sb->block_size;
    u32 name_len = strlen(name);
    u32 need_len = sizeof(struct ext2_dirent) + name_len;
    need_len = align_up(need_len, 4); // 4‑byte alignment

    u8 *block = malloc(block_size);
    if (!block)
        return -1;

    /* Walk directory blocks */
    for (u32 iblock = 0; iblock < 12; iblock++)
    {
        u32 blk = dir_inode->block[iblock];

        if (blk == 0)
        {
            /* Allocate a new block for the directory */
            if (ext2_alloc_block(sb, &blk) != 0)
            {
                free(block);
                return -1;
            }

            dir_inode->block[iblock] = blk;
            dir_inode->size = (iblock + 1) * block_size;
            dir_inode->blocks += block_size / 512;

            memset(block, 0, block_size);

            /* Create the first entry in this new block */
            struct ext2_dirent *de = (struct ext2_dirent *)block;
            de->ino = child_ino;
            de->name_len = name_len;
            de->file_type = file_type;
            de->rec_len = block_size;

            memcpy(de->name, name, name_len);

            ext2_write_block(sb, blk, block);
            free(block);
            return 0;
        }

        /* Read existing block */
        ext2_read_block(sb, blk, block);

        /* Walk entries to find the last one */
        u32 off = 0;
        struct ext2_dirent *de = NULL;

        while (off < block_size)
        {
            de = (struct ext2_dirent *)(block + off);

            if (de->rec_len == 0)
            {
                free(block);
                return -1; // corrupted directory
            }

            /* Last entry? */
            if (off + de->rec_len >= block_size)
                break;

            off += de->rec_len;
        }

        /* Compute actual size of last entry */
        u32 actual_len = sizeof(struct ext2_dirent) + de->name_len;
        actual_len = align_up(actual_len, 4);

        u32 slack = de->rec_len - actual_len;

        if (slack >= need_len)
        {
            /* Shrink last entry */
            de->rec_len = actual_len;

            /* Insert new entry in the slack space */
            struct ext2_dirent *newde =
                (struct ext2_dirent *)((u8 *)de + actual_len);

            newde->ino = child_ino;
            newde->name_len = name_len;
            newde->file_type = file_type;
            newde->rec_len = slack;

            memcpy(newde->name, name, name_len);

            ext2_write_block(sb, blk, block);
            free(block);
            return 0;
        }

        /* No space in this block — continue to next */
    }

    /* No space in direct blocks — no indirect support yet */
    free(block);
    return -1;
}

static int ext2_create_vn(struct vnode *dir, const char *name,
                          vnode_type_t type, struct vnode **out)
{
    ext2_node_t *dnode = (ext2_node_t *)dir->fs_data;
    ext2_super_t *sb = dnode->sb;
    ext2_inode_disk_t *dinode = &dnode->inode;

    u16 mode;
    u8 file_type;

    if (type == VNODE_TYPE_DIR)
    {
        mode = 0x4000; /* directory */
        file_type = 2; /* EXT2_FT_DIR */
    }
    else
    {
        mode = 0x8000; /* regular file */
        file_type = 1; /* EXT2_FT_REG_FILE */
    }

    /* 1. Allocate inode */
    u32 ino;
    if (ext2_alloc_inode(sb, &ino, mode) != 0)
        return -1;

    /* 2. Load fresh inode */
    ext2_inode_disk_t new_inode;
    ext2_read_inode(sb, ino, &new_inode);

    /* 3. If directory, allocate block and write "." and ".." */
    if (type == VNODE_TYPE_DIR)
    {
        u32 blk;
        if (ext2_alloc_block(sb, &blk) != 0)
            return -1;

        new_inode.block[0] = blk;
        new_inode.size = sb->block_size;
        new_inode.blocks = sb->block_size / 512;

        /* dir link count = 2 (".", "..") */
        new_inode.links_count = 2;
        /* parent gets +1 link for this subdir */
        dinode->links_count++;

        u8 *block = malloc(sb->block_size);
        if (!block)
            return -1;
        memset(block, 0, sb->block_size);

        /* "." */
        struct ext2_dirent *de = (struct ext2_dirent *)block;
        de->ino = ino;
        de->name_len = 1;
        de->file_type = 2;
        u32 rec_len = sizeof(struct ext2_dirent) + 1;
        rec_len = align_up(rec_len, 4);
        de->rec_len = rec_len;
        de->name[0] = '.';

        /* ".." */
        struct ext2_dirent *de2 = (struct ext2_dirent *)(block + rec_len);
        de2->ino = dnode->ino;
        de2->name_len = 2;
        de2->file_type = 2;
        de2->rec_len = sb->block_size - rec_len;
        de2->name[0] = '.';
        de2->name[1] = '.';

        ext2_write_block(sb, blk, block);
        free(block);
    }

    /* 4. Write new inode */
    ext2_write_inode(sb, ino, &new_inode);

    /* 5. Insert entry into parent directory */
    if (ext2_dir_add_entry(sb, dinode, ino, name, file_type) != 0)
        return -1;

    /* 6. Write updated parent inode (size/link count may have changed) */
    ext2_write_inode(sb, dnode->ino, dinode);

    /* 7. Allocate vnode */
    *out = ext2_alloc_vnode(dir->mount, sb, ino, &new_inode);
    return 0;
}

static int ext2_write_vn(struct vnode *vn, const void *buf,
                         usize off, usize len, usize *out)
{
    ext2_node_t *node = (ext2_node_t *)vn->fs_data;
    ext2_super_t *sb = node->sb;
    ext2_inode_disk_t *inode = &node->inode;

    if (vn->type != VNODE_TYPE_FILE)
        return -1;

    const u8 *src = (const u8 *)buf;
    usize done = 0;

    u8 *block = (u8 *)malloc(sb->block_size);
    if (!block)
        return -1;

    while (done < len)
    {
        u32 file_off = off + done;
        u32 iblock = file_off / sb->block_size;
        u32 blk_off = file_off % sb->block_size;
        usize to_copy = sb->block_size - blk_off;
        if (to_copy > len - done)
            to_copy = len - done;

        /* only direct blocks for now */
        if (iblock >= 12)
        {
            break;
        }

        /* allocate block if missing */
        if (inode->block[iblock] == 0)
        {
            u32 newblk;
            if (ext2_alloc_block(sb, &newblk) != 0)
                break;
            inode->block[iblock] = newblk;
            inode->blocks += sb->block_size / 512;
        }

        u32 blk = inode->block[iblock];

        /* read‑modify‑write the block */
        if (ext2_read_block(sb, blk, block) != 0)
            break;

        memcpy(block + blk_off, src + done, to_copy);

        if (ext2_write_block(sb, blk, block) != 0)
            break;

        done += to_copy;
    }

    free(block);

    /* update size if we extended the file */
    if (off + done > inode->size)
        inode->size = off + done;

    /* write inode back */
    ext2_write_inode(sb, node->ino, inode);

    *out = done;
    return 0;
}

static int ext2_unlink_vn(struct vnode *dir, const char *name)
{
    // TODO:
    // 1. Lookup entry
    // 2. Remove directory entry (set ino=0 or shrink rec_len)
    // 3. Decrement link count
    // 4. Free inode if link count == 0
    // 5. Free blocks

    (void)dir;
    (void)name;
    return -1;
}

static int ext2_truncate_vn(struct vnode *vn, usize new_size)
{
    // TODO:
    // 1. If shrinking: free blocks beyond new_size
    // 2. If growing: allocate blocks
    // 3. Update inode size
    // 4. Write inode back

    (void)vn;
    (void)new_size;
    return -1;
}

static int ext2_getattr_vn(struct vnode *vn, void *stat_buf)
{
    return 0;
}

static int ext2_setattr_vn(struct vnode *vn, const void *stat_buf)
{
    // TODO:
    // 1. Update inode metadata (mode, uid, gid, times)
    // 2. Write inode back

    (void)vn;
    (void)stat_buf;
    return -1;
}
