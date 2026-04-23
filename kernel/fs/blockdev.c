#include "fs/blockdev.h"
#include "drivers/ata.h"
#include "libk/printf.h"
#include "libk/mem.h"

// Forward declarations of ATA read/write
extern int ata_read28(u32 lba, void *buf, u32 count);
extern int ata_write28(u32 lba, const void *buf, u32 count);

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
    .read  = ata_block_read,
    .write = ata_block_write,
    .sector_size = 512,
    .priv = NULL,
};

struct blockdev *blockdev_open(const char *name)
{
    if (!strcmp(name, "ata0") || !strcmp(name, "hd0"))
        return &ata0;

    printf("blockdev_open: unknown device '%s'\n", name);
    return NULL;
}
