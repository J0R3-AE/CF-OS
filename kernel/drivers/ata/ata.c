
#include "arch/io.h"

#include "libk/printf.h"
#include "libk/log.h"

#include "drivers/ata.h"

static int ata_wait_bsy(void)
{
    while (io_Read8(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_BSY)
        ;
    return 0;
}

static int ata_wait_drq(void)
{
    while (!(io_Read8(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_DRQ))
        ;
    return 0;
}

int ata_identify(void)
{
    io_Write8(ATA_PRIMARY_CTRL, 0x00); // enable IRQs

    io_Write8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0); // master drive
    io_wait();

    io_Write8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 0);
    io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA0, 0);
    io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA1, 0);
    io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA2, 0);

    io_Write8(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    u8 status = io_Read8(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status == 0)
    {
        klog_warn("ATA: no device");
        return -1;
    }

    ata_wait_bsy();

    status = io_Read8(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status & ATA_SR_ERR)
    {
        klog_warn("ATA: error");
        return -1;
    }

    ata_wait_drq();

    u16 id_data[256];
    for (int i = 0; i < 256; i++)
        id_data[i] = io_Read16(ATA_PRIMARY_IO + ATA_REG_DATA);

    // Extract model string (words 27–46)
    char model[41];
    for (int i = 0; i < 20; i++)
    {
        model[i * 2] = id_data[27 + i] >> 8;
        model[i * 2 + 1] = id_data[27 + i] & 0xFF;
    }
    model[40] = 0;

    klog_info("ATA Model: %s", model);

    u32 sectors =
        ((u32)id_data[60]) |
        ((u32)id_data[61] << 16);

    klog_info("ATA Size: %u sectors (%u MB)",
              sectors, sectors / 2048);

    return 0;
}

int ata_read28(u32 lba, void *buf, u32 count)
{
    for (u32 i = 0; i < count; i++)
    {
        // Wait until not busy
        ata_wait_bsy();

        // Select drive + LBA mode
        io_Write8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA0, (u8)(lba));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA1, (u8)(lba >> 8));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA2, (u8)(lba >> 16));

        // Read command
        io_Write8(ATA_PRIMARY_IO + ATA_REG_COMMAND, 0x20);

        ata_wait_bsy();
        ata_wait_drq();

        u16 *ptr = (u16 *)((u8 *)buf + i * 512);
        for (int j = 0; j < 256; j++)
            ptr[j] = io_Read16(ATA_PRIMARY_IO + ATA_REG_DATA);

        lba++;
    }

    return 0;
}

int ata_write28(u32 lba, const void *buf, u32 count)
{
    for (u32 i = 0; i < count; i++)
    {
        ata_wait_bsy();

        io_Write8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA0, (u8)(lba));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA1, (u8)(lba >> 8));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA2, (u8)(lba >> 16));

        io_Write8(ATA_PRIMARY_IO + ATA_REG_COMMAND, 0x30);

        ata_wait_bsy();
        ata_wait_drq();

        const u16 *ptr = (const u16 *)((const u8 *)buf + i * 512);
        for (int j = 0; j < 256; j++)
            io_Write16(ATA_PRIMARY_IO + ATA_REG_DATA, ptr[j]);

        lba++;
    }

    return 0;
}
