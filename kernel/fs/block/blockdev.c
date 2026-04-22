#include "fs/blockdev.h"
#include "drivers/ata.h"
#include "libk/printf.h"
#include "libk/mem.h"

// Forward declarations of ATA read/write
extern int ata_read28(u32 lba, void *buf, u32 count);
extern int ata_write28(u32 lba, const void *buf, u32 count);

#define MAX_BLOCKDEVS 8

static struct blockdev *devices[MAX_BLOCKDEVS];
static const char *names[MAX_BLOCKDEVS];

static int dev_count = 0;

static int ata_block_read(struct blockdev *bd, u32 lba, u32 count, void *buf)
{
    (void)bd;
    return ata_read28(lba, buf, count);
}

static int ata_block_write(struct blockdev *bd, u32 lba, u32 count, const void *buf)
{
    (void)bd;
    return ata_write28(lba, buf, count);
}

static struct blockdev ata0 = {
    .read = ata_block_read,
    .write = ata_block_write,
    .sector_size = 512,
    .priv = NULL,
};

struct blockdev *blockdev_open(const char *name)
{
    for (int i = 0; i < dev_count; i++)
    {
        if (!strcmp(names[i], name))
            return devices[i];
    }

    printf("blockdev_open: unknown device '%s'\n", name);
    return NULL;
}

void blockdev_register(const char *name, struct blockdev *dev)
{
    if (dev_count >= MAX_BLOCKDEVS)
        return;

    names[dev_count] = name;
    devices[dev_count] = dev;
    dev_count++;
}
