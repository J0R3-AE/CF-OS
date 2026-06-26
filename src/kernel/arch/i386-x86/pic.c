#include "arch/pic.h"
#include "arch/io.h"
#include <stdint.h>
#include "libk/log.h"

void pic_remap(int offset1, int offset2)
{
    uint8_t mask1 = io_Read8(PIC1_REG_IMR);
    uint8_t mask2 = io_Read8(PIC2_REG_IMR);

    /* Start initialization */
    io_Write8(PIC_MASTER_CMD, PIC_ICW1_INIT_YES | PIC_ICW1_IC4_EXPECT);
    io_Write8(PIC_SLAVE_CMD, PIC_ICW1_INIT_YES | PIC_ICW1_IC4_EXPECT);

    /* Remap IRQs */
    io_Write8(PIC_MASTER_DATA, offset1); /* master offset */
    io_Write8(PIC_SLAVE_DATA, offset2);  /* slave offset */

    /* Tell Master PIC that there is a slave PIC at IRQ2 */
    io_Write8(PIC_MASTER_DATA, 0x04);

    /* Tell Slave PIC its cascade identity */
    io_Write8(PIC_SLAVE_DATA, 0x02);

    /* 8086 mode */
    io_Write8(PIC_MASTER_DATA, PIC_ICW4_UPM_86MODE);
    io_Write8(PIC_SLAVE_DATA, PIC_ICW4_UPM_86MODE);

    /* Restore masks */
    io_Write8(PIC1_REG_IMR, mask1);
    io_Write8(PIC2_REG_IMR, mask2);
}

void pic_init(void)
{
    pic_remap(0x20, 0x28); /* Remap PIC1 to 0x20-0x27 and PIC2 to 0x28-0x2F */
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
    {
        io_Write8(PIC_SLAVE_CMD, PIC_CMD_EOI);
    }
    io_Write8(PIC_MASTER_CMD, PIC_CMD_EOI);
}

void pic_mask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8)
    {
        port = PIC1_REG_IMR;
    }
    else
    {
        port = PIC2_REG_IMR;
        irq -= 8;
    }

    value = io_Read8(port) | (1 << irq);
    io_Write8(port, value);
}

void pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8)
    {
        port = PIC1_REG_IMR;
    }
    else
    {
        port = PIC2_REG_IMR;
        irq -= 8;
    }

    value = io_Read8(port) & ~(1 << irq);
    io_Write8(port, value);
}