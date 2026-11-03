#include "kernel/arch/io.h"
#include "kernel/drivers/ata.h"
#include "kernel/fs/blockdev.h"
#include "libc/errno.h"
#include "libc/mem.h"
#include "libc/types.h"

#define ATA_SECTOR_SIZE 512u
#define ATA_LBA28_SECTOR_LIMIT 0x10000000u
#define ATA_POLL_LIMIT 1000000u

static u32 g_ata_sector_count;
static struct blockdev g_ata_blockdev;

static int ata_wait_not_busy(void)
{
    for (u32 poll = 0; poll < ATA_POLL_LIMIT; poll++)
    {
        u8 status = in8(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (status == 0 || status == 0xFF)
            return ERR_DEVICE_NOT_FOUND;
        if (!(status & ATA_SR_BSY))
            return (status & (ATA_SR_ERR | 0x20u))
                       ? ERR_IO_ERROR
                       : ERR_SUCCESS;
    }

    return ERR_TIMED_OUT;
}

static int ata_wait_drq(void)
{
    for (u32 poll = 0; poll < ATA_POLL_LIMIT; poll++)
    {
        u8 status = in8(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (status == 0 || status == 0xFF)
            return ERR_DEVICE_NOT_FOUND;
        if (status & (ATA_SR_ERR | 0x20u))
            return ERR_IO_ERROR;
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
            return ERR_SUCCESS;
    }

    return ERR_TIMED_OUT;
}

static void ata_delay_400ns(void)
{
    (void)in8(ATA_PRIMARY_CTRL);
    (void)in8(ATA_PRIMARY_CTRL);
    (void)in8(ATA_PRIMARY_CTRL);
    (void)in8(ATA_PRIMARY_CTRL);
}

int ata_identify(void)
{
    u16 identify_data[256];
    u8 status;

    g_ata_sector_count = 0;
    out8(ATA_PRIMARY_CTRL, 0x02);
    out8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0);
    ata_delay_400ns();
    out8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 0);
    out8(ATA_PRIMARY_IO + ATA_REG_LBA0, 0);
    out8(ATA_PRIMARY_IO + ATA_REG_LBA1, 0);
    out8(ATA_PRIMARY_IO + ATA_REG_LBA2, 0);
    out8(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    status = in8(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status == 0 || status == 0xFF)
        return ERR_DEVICE_NOT_FOUND;

    int result = ata_wait_not_busy();
    if (result != ERR_SUCCESS)
        return result;

    if (in8(ATA_PRIMARY_IO + ATA_REG_LBA1) != 0 ||
        in8(ATA_PRIMARY_IO + ATA_REG_LBA2) != 0)
        return ERR_NOT_SUPPORTED;

    result = ata_wait_drq();
    if (result != ERR_SUCCESS)
        return result;

    for (u32 i = 0; i < 256; i++)
        identify_data[i] = in16(ATA_PRIMARY_IO + ATA_REG_DATA);

    if (!(identify_data[49] & (1u << 9)))
        return ERR_NOT_SUPPORTED;

    g_ata_sector_count = (u32)identify_data[60] |
                         ((u32)identify_data[61] << 16);
    if (!g_ata_sector_count)
        return ERR_IO_ERROR;
    if (g_ata_sector_count > ATA_LBA28_SECTOR_LIMIT)
        g_ata_sector_count = ATA_LBA28_SECTOR_LIMIT;

    return ERR_SUCCESS;
}

u32 ata_sector_count(void)
{
    return g_ata_sector_count;
}

int ata_read28(u32 lba, void *buf, u32 count)
{
    if (!count)
        return ERR_SUCCESS;
    if (!buf || !g_ata_sector_count || lba >= g_ata_sector_count ||
        count > g_ata_sector_count - lba)
        return ERR_INVALID_ARGUMENT;

    for (u32 sector = 0; sector < count; sector++)
    {
        int result = ata_wait_not_busy();
        if (result != ERR_SUCCESS)
            return result;

        u32 current_lba = lba + sector;
        out8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL,
             (u8)(0xE0 | ((current_lba >> 24) & 0x0F)));
        ata_delay_400ns();
        out8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
        out8(ATA_PRIMARY_IO + ATA_REG_LBA0, (u8)current_lba);
        out8(ATA_PRIMARY_IO + ATA_REG_LBA1, (u8)(current_lba >> 8));
        out8(ATA_PRIMARY_IO + ATA_REG_LBA2, (u8)(current_lba >> 16));
        out8(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ28);

        result = ata_wait_drq();
        if (result != ERR_SUCCESS)
            return result;

        u8 *destination = (u8 *)buf + sector * ATA_SECTOR_SIZE;
        for (u32 word = 0; word < ATA_SECTOR_SIZE / sizeof(u16); word++)
        {
            u16 value = in16(ATA_PRIMARY_IO + ATA_REG_DATA);
            destination[word * 2] = (u8)value;
            destination[word * 2 + 1] = (u8)(value >> 8);
        }
    }

    return ERR_SUCCESS;
}

int ata_write28(u32 lba, const void *buf, u32 count)
{
    if (!count)
        return ERR_SUCCESS;
    if (!buf || !g_ata_sector_count || lba >= g_ata_sector_count ||
        count > g_ata_sector_count - lba)
        return ERR_INVALID_ARGUMENT;

    for (u32 sector = 0; sector < count; sector++)
    {
        int result = ata_wait_not_busy();
        if (result != ERR_SUCCESS)
            return result;

        u32 current_lba = lba + sector;
        out8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL,
             (u8)(0xE0 | ((current_lba >> 24) & 0x0F)));
        ata_delay_400ns();
        out8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
        out8(ATA_PRIMARY_IO + ATA_REG_LBA0, (u8)current_lba);
        out8(ATA_PRIMARY_IO + ATA_REG_LBA1, (u8)(current_lba >> 8));
        out8(ATA_PRIMARY_IO + ATA_REG_LBA2, (u8)(current_lba >> 16));
        out8(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE28);

        result = ata_wait_drq();
        if (result != ERR_SUCCESS)
            return result;

        const u8 *source = (const u8 *)buf + sector * ATA_SECTOR_SIZE;
        for (u32 word = 0; word < ATA_SECTOR_SIZE / sizeof(u16); word++)
        {
            u16 value = (u16)source[word * 2] |
                        ((u16)source[word * 2 + 1] << 8);
            out16(ATA_PRIMARY_IO + ATA_REG_DATA, value);
        }

        result = ata_wait_not_busy();
        if (result != ERR_SUCCESS)
            return result;
    }

    out8(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_FLUSH);
    return ata_wait_not_busy();
}

static int ata_blockdev_read(struct blockdev *bd, u32 lba, u32 count,
                             void *buf)
{
    (void)bd;
    return ata_read28(lba, buf, count);
}

static int ata_blockdev_write(struct blockdev *bd, u32 lba, u32 count,
                              const void *buf)
{
    (void)bd;
    return ata_write28(lba, buf, count);
}

static const struct blockdev_ops g_ata_blockdev_ops = {
    .read = ata_blockdev_read,
    .write = ata_blockdev_write,
};

int ata_init(void)
{
    int result = ata_identify();
    if (result != ERR_SUCCESS)
        return result;

    g_ata_blockdev.name = "hd0";
    g_ata_blockdev.ops = &g_ata_blockdev_ops;
    g_ata_blockdev.sector_size = ATA_SECTOR_SIZE;
    g_ata_blockdev.sector_count = g_ata_sector_count;
    g_ata_blockdev.private_data = NULL;

    return blockdev_register(&g_ata_blockdev);
}

#ifdef ATA_PIO_SELFTEST
int ata_selftest(void)
{
    u8 original[ATA_SECTOR_SIZE];
    u8 pattern[ATA_SECTOR_SIZE];
    u8 actual[ATA_SECTOR_SIZE];
    int result;
    int restore_result;

    result = ata_read28(0, original, 1);
    if (result != ERR_SUCCESS)
        return result;

    for (u32 i = 0; i < ATA_SECTOR_SIZE; i++)
        pattern[i] = (u8)(i ^ 0xA5u);

    result = ata_write28(0, pattern, 1);
    if (result == ERR_SUCCESS)
    {
        result = ata_read28(0, actual, 1);
        if (result == ERR_SUCCESS && memcmp(pattern, actual, sizeof(pattern)) != 0)
            result = ERR_IO_ERROR;
    }

    restore_result = ata_write28(0, original, 1);
    if (restore_result != ERR_SUCCESS)
        return restore_result;

    restore_result = ata_read28(0, actual, 1);
    if (restore_result != ERR_SUCCESS)
        return restore_result;
    if (memcmp(original, actual, sizeof(original)) != 0)
        return ERR_IO_ERROR;

    return result;
}
#endif
