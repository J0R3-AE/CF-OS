#include "kernel/arch/io.h"

#include "libc/printf.h"

#include "kernel/drivers/ata.h"

static int ata_wait_bsy(void)
{
    for (int i = 0; i < 1000000; i++)
        if (!(in8(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_BSY))
            return 0;

    return -1;
}

static int ata_wait_drq(void)
{
    for (int i = 0; i < 1000000; i++)
        if (in8(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_DRQ)
            return 0;

    return -1;
}

int ata_identify(void)
{
    out8(ATA_PRIMARY_CTRL, 0x00); /* enable IRQs */

    out8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0); /* master drive */
    
    wait();

    out8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 0);
    out8(ATA_PRIMARY_IO + ATA_REG_LBA0, 0);
    out8(ATA_PRIMARY_IO + ATA_REG_LBA1, 0);
    out8(ATA_PRIMARY_IO + ATA_REG_LBA2, 0);

    out8(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    u8 status = in8(ATA_PRIMARY_IO + ATA_REG_STATUS);

    if (status == 0) return -1;

    if (ata_wait_bsy() < 0) return -1;

    status = in8(ATA_PRIMARY_IO + ATA_REG_STATUS);

    if (status & ATA_SR_ERR) return -1;
    

    if (ata_wait_drq() < 0) return -1;

    u16 id_data[256];
    for (int i = 0; i < 256; i++){id_data[i] = in16(ATA_PRIMARY_IO + ATA_REG_DATA);}

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
    return 0;
}

int ata_read28(u32 lba, void *buf, u32 count)
{
    u32 start_lba = lba;

    for (u32 i = 0; i < count; i++)
    {
        if (ata_wait_bsy() < 0) return -1;

        out8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
        out8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
        out8(ATA_PRIMARY_IO + ATA_REG_LBA0, (u8)(lba));
        out8(ATA_PRIMARY_IO + ATA_REG_LBA1, (u8)(lba >> 8));
        out8(ATA_PRIMARY_IO + ATA_REG_LBA2, (u8)(lba >> 16));

        out8(ATA_PRIMARY_IO + ATA_REG_COMMAND, 0x20);

        if (ata_wait_bsy() < 0) return -1;
        if (ata_wait_drq() < 0) return -1;

        u16 *ptr = (u16 *)((u8 *)buf + i * 512);

        for (int j = 0; j < 256; j++){ptr[j] = in16(ATA_PRIMARY_IO + ATA_REG_DATA);}

        lba++;
    }
    return 0;
}

int ata_write28(u32 lba, const void *buf, u32 count)
{
    u32 start_lba = lba;

    for (u32 i = 0; i < count; i++)
    {
        if (ata_wait_bsy() < 0) return -1;

        out8(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
        out8(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
        out8(ATA_PRIMARY_IO + ATA_REG_LBA0, (u8)(lba));
        out8(ATA_PRIMARY_IO + ATA_REG_LBA1, (u8)(lba >> 8));
        out8(ATA_PRIMARY_IO + ATA_REG_LBA2, (u8)(lba >> 16));

        out8(ATA_PRIMARY_IO + ATA_REG_COMMAND, 0x30);

        if (ata_wait_bsy() < 0) return -1;
        if (ata_wait_drq() < 0) return -1;

        const u16 *ptr = (const u16 *)((const u8 *)buf + i * 512);

        for (int j = 0; j < 256; j++){out16(ATA_PRIMARY_IO + ATA_REG_DATA, ptr[j]);}

        lba++;
    }
    return 0;
}