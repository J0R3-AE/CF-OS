#include "arch/pic.h"
#include "arch/io.h"
#include <stdint.h>
#include "libk/log.h"

void pic_init(void)
{
    uint8_t mask1 = io_Read8(PIC1_REG_IMR);
    uint8_t mask2 = io_Read8(PIC2_REG_IMR);
    klog_log("PIC: Current masks before init: master=0x%x slave=0x%x", mask1, mask2);

    /* Start initialization */
    io_Write8(PIC_MASTER_CMD, PIC_ICW1_INIT_YES | PIC_ICW1_IC4_EXPECT);
    io_Write8(PIC_SLAVE_CMD, PIC_ICW1_INIT_YES | PIC_ICW1_IC4_EXPECT);
    klog_log("PIC: Sent ICW1 to start initialization");

    /* Remap IRQs */
    io_Write8(PIC_MASTER_DATA, 0x20); /* master offset = 32 */
    io_Write8(PIC_SLAVE_DATA, 0x28);  /* slave offset  = 40 */
    klog_log("PIC: Remapped IRQs to 0x20-0x2F");

    /* Tell Master PIC that there is a slave PIC at IRQ2 */
    io_Write8(PIC_MASTER_DATA, 0x04);
    klog_log("PIC: Told Master PIC about slave PIC");


    /* Tell Slave PIC its cascade identity */
    io_Write8(PIC_SLAVE_DATA, 0x02);
    klog_log("PIC: Set Slave PIC cascade identity");

    /* 8086 mode */
    io_Write8(PIC_MASTER_DATA, PIC_ICW4_UPM_86MODE);
    io_Write8(PIC_SLAVE_DATA, PIC_ICW4_UPM_86MODE);
    klog_log("PIC: Set 8086 mode");

    /* Restore masks */
    io_Write8(PIC1_REG_IMR, mask1);
    io_Write8(PIC2_REG_IMR, mask2);
    klog_log("PIC initialized and remapped to 0x20-0x2F");
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
    klog_log("PIC: Masked IRQ %u (port=0x%x)", irq, port);
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
    klog_log("PIC: Unmasked IRQ %u (port=0x%x)", irq, port);
}