#include "drivers/ramdisk.h"
#include "libk/mem.h"
#include "libk/log.h"

/* simple in-memory block device */

static struct ramdisk_device *rd = NULL;

static int rd_read(struct blockdev *bd, u32 lba, u32 count, void *buf)
{
    struct ramdisk_device *r = bd->priv;

    u32 offset = lba * bd->sector_size;
    u32 bytes = count * bd->sector_size;

    if (offset + bytes > r->size)
        return -1;

    memcpy(buf, (u8 *)r->image + offset, bytes);
    return 0;
}
static int rd_write(struct blockdev *bd, u32 lba, u32 count, const void *buf)
{
    struct ramdisk_device *r = bd->priv;

    u32 offset = lba * bd->sector_size;
    u32 bytes = count * bd->sector_size;

    if (offset + bytes > r->size)
        return -1;

    memcpy((u8 *)r->image + offset, buf, bytes);
    return 0;
}

struct blockdev *ramdisk_init(void *data, u32 size, const char *name)
{
    struct ramdisk_device *rd = malloc(sizeof(*rd));
    rd->image = data;
    rd->size = size;

    struct blockdev *dev = malloc(sizeof(*dev));

    dev->read = rd_read;
    dev->write = rd_write;
    dev->sector_size = 512;
    dev->priv = rd;

    blockdev_register(name, dev);

    klog_info("ramdisk: registered as %s", name);
    return dev;
}