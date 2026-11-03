#include "kernel/fs/partition.h"

#include "libc/errno.h"
#include "libc/mem.h"
#include "libc/string.h"

#define MBR_SIGNATURE_OFFSET 510u
#define MBR_PARTITION_TABLE_OFFSET 446u
#define MBR_PARTITION_ENTRY_SIZE 16u
#define MBR_PARTITION_COUNT 4u

struct mbr_partition
{
    struct blockdev blockdev;
    struct blockdev *disk;
    u32 start_lba;
};

static struct mbr_partition g_partitions[MBR_PARTITION_COUNT];
static struct blockdev *g_partition_disk;
static const char *g_partition_names[MBR_PARTITION_COUNT] = {
    "hd0p1", "hd0p2", "hd0p3", "hd0p4"
};

static u32 partition_read_u32(const u8 *bytes)
{
    return (u32)bytes[0] |
           ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) |
           ((u32)bytes[3] << 24);
}

static int partition_read(struct blockdev *device, u32 lba, u32 count,
                          void *buffer)
{
    struct mbr_partition *partition = device->private_data;
    if (!partition || !partition->disk || lba >= device->sector_count ||
        count > device->sector_count - lba)
        return ERR_INVALID_ARGUMENT;

    return blockdev_read(partition->disk, partition->start_lba + lba,
                         count, buffer);
}

static int partition_write(struct blockdev *device, u32 lba, u32 count,
                           const void *buffer)
{
    struct mbr_partition *partition = device->private_data;
    if (!partition || !partition->disk || lba >= device->sector_count ||
        count > device->sector_count - lba)
        return ERR_INVALID_ARGUMENT;

    return blockdev_write(partition->disk, partition->start_lba + lba,
                          count, buffer);
}

static const struct blockdev_ops g_partition_ops = {
    .read = partition_read,
    .write = partition_write,
};

int partition_scan(struct blockdev *disk)
{
    u8 sector[512];
    u32 starts[MBR_PARTITION_COUNT] = {0};
    u32 counts[MBR_PARTITION_COUNT] = {0};
    int result;
    u32 found = 0;

    if (!disk || disk->sector_size != sizeof(sector) ||
        disk->sector_count == 0 || !disk->name ||
        strcmp(disk->name, "hd0") != 0)
        return ERR_INVALID_ARGUMENT;

    if (g_partition_disk)
    {
        if (g_partition_disk != disk)
            return ERR_BUSY;
        u32 registered = 0;
        for (u32 i = 0; i < MBR_PARTITION_COUNT; i++)
            if (g_partitions[i].blockdev.name)
                registered++;
        return (int)registered;
    }

    result = blockdev_read(disk, 0, 1, sector);
    if (result != ERR_SUCCESS)
        return result;
    if (sector[MBR_SIGNATURE_OFFSET] != 0x55 ||
        sector[MBR_SIGNATURE_OFFSET + 1] != 0xAA)
        return ERR_NOT_FOUND;

    g_partition_disk = disk;
    for (u32 i = 0; i < MBR_PARTITION_COUNT; i++)
    {
        const u8 *entry = sector + MBR_PARTITION_TABLE_OFFSET +
                          i * MBR_PARTITION_ENTRY_SIZE;
        u8 status = entry[0];
        u8 type = entry[4];
        u32 start_lba = partition_read_u32(entry + 8);
        u32 sector_count = partition_read_u32(entry + 12);
        struct mbr_partition *partition;
        int overlaps = 0;

        if (type == 0 || !sector_count)
            continue;
        if ((status != 0 && status != 0x80) ||
            type == 0x05 || type == 0x0F || type == 0x85)
            continue;
        if (!start_lba || start_lba >= disk->sector_count ||
            sector_count > disk->sector_count - start_lba)
            continue;
        for (u32 j = 0; j < i; j++)
        {
            if (counts[j] && start_lba < starts[j] + counts[j] &&
                starts[j] < start_lba + sector_count)
            {
                overlaps = 1;
                break;
            }
        }
        if (overlaps)
            continue;

        partition = &g_partitions[i];
        partition->disk = disk;
        partition->start_lba = start_lba;
        partition->blockdev.name = g_partition_names[i];
        partition->blockdev.ops = &g_partition_ops;
        partition->blockdev.sector_size = disk->sector_size;
        partition->blockdev.sector_count = sector_count;
        partition->blockdev.private_data = partition;

        result = blockdev_register(&partition->blockdev);
        if (result != ERR_SUCCESS)
        {
            for (u32 rollback = 0; rollback < MBR_PARTITION_COUNT; rollback++)
            {
                if (g_partitions[rollback].blockdev.name)
                {
                    blockdev_unregister(&g_partitions[rollback].blockdev);
                    memset(&g_partitions[rollback], 0,
                           sizeof(g_partitions[rollback]));
                }
            }
            g_partition_disk = NULL;
            return result;
        }
        starts[i] = start_lba;
        counts[i] = sector_count;
        found++;
    }

    return (int)found;
}