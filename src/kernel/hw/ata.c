#include "arch/io.h"

#include "libk/printf.h"
#include "libk/log.h"

#include "drivers/ata.h"

static int ata_wait_bsy(void)
{
    for (int i = 0; i < 1000000; i++)
        if (!(io_Read8(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_BSY))
            return 0;

    return -1;
}

static int ata_wait_drq(void)
{
    for (int i = 0; i < 1000000; i++)
        if (io_Read8(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_DRQ)
            return 0;

    return -1;
}

int ata_identify(void)
{
    io_Write8(ATA_PRIMARY_CTRL, 0x00); /* enable IRQs */

    io_Write8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0); /* master drive */
    
    io_wait();

    io_Write8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 0);
    io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA0, 0);
    io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA1, 0);
    io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA2, 0);

    io_Write8(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    u8 status = io_Read8(ATA_PRIMARY_IO + ATA_REG_STATUS);

    if (status == 0) return -1;

    if (ata_wait_bsy() < 0) return -1;

    status = io_Read8(ATA_PRIMARY_IO + ATA_REG_STATUS);

    if (status & ATA_SR_ERR) return -1;
    

    if (ata_wait_drq() < 0) return -1;

    u16 id_data[256];
    for (int i = 0; i < 256; i++){id_data[i] = io_Read16(ATA_PRIMARY_IO + ATA_REG_DATA);}

    /* Extract model string (words 27–46) */
    char model[41];

    for (int i = 0; i < 20; i++)
    {
        model[i * 2] = (char)((id_data[27 + i] >> 8) & 0xFF);
        model[i * 2 + 1] = (char)(id_data[27 + i] & 0xFF);
    }

    model[40] = 0;

    /* trim trailing spaces */
    for (int i = 39; i >= 0; i--)
    {
        if (model[i] == ' ' || model[i] == '\0'){model[i] = 0;}
        else break;
    }

    u32 sectors =((u32)id_data[60])|((u32)id_data[61] << 16);

    KLOG_LOG("ATA: model='%s' sectors=%u", model, sectors);
    return 0;
}

int ata_read28(u32 lba, void *buf, u32 count)
{
    u32 start_lba = lba;

    for (u32 i = 0; i < count; i++)
    {
        if (ata_wait_bsy() < 0) return -1;

        io_Write8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA0, (u8)(lba));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA1, (u8)(lba >> 8));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA2, (u8)(lba >> 16));

        io_Write8(ATA_PRIMARY_IO + ATA_REG_COMMAND, 0x20);

        if (ata_wait_bsy() < 0) return -1;
        if (ata_wait_drq() < 0) return -1;

        u16 *ptr = (u16 *)((u8 *)buf + i * 512);

        for (int j = 0; j < 256; j++){ptr[j] = io_Read16(ATA_PRIMARY_IO + ATA_REG_DATA);}

        lba++;
    }
    KLOG_OKAY("ata_read28: read %u sectors starting at LBA %u into buffer %p", count, start_lba, buf);
    return 0;
}

int ata_write28(u32 lba, const void *buf, u32 count)
{
    u32 start_lba = lba;

    for (u32 i = 0; i < count; i++)
    {
        if (ata_wait_bsy() < 0) return -1;

        io_Write8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA0, (u8)(lba));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA1, (u8)(lba >> 8));
        io_Write8(ATA_PRIMARY_IO + ATA_REG_LBA2, (u8)(lba >> 16));

        io_Write8(ATA_PRIMARY_IO + ATA_REG_COMMAND, 0x30);

        if (ata_wait_bsy() < 0) return -1;
        if (ata_wait_drq() < 0) return -1;

        const u16 *ptr = (const u16 *)((const u8 *)buf + i * 512);

        for (int j = 0; j < 256; j++){io_Write16(ATA_PRIMARY_IO + ATA_REG_DATA, ptr[j]);}

        lba++;
    }

    KLOG_OKAY("ata_write28: wrote %u sectors starting at LBA %u from buffer %p", count, start_lba, buf);
    return 0;
}