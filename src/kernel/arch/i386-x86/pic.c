#include "kernel/arch/pic.h"
#include "kernel/arch/io.h"
#include <stdint.h>

void pic_remap(int offset1, int offset2)
{
    uint8_t mask1 = in8(PIC1_REG_IMR);
    uint8_t mask2 = in8(PIC2_REG_IMR);

    /* Start initialization */
    out8(PIC_MASTER_CMD, PIC_ICW1_INIT_YES | PIC_ICW1_IC4_EXPECT);
    out8(PIC_SLAVE_CMD, PIC_ICW1_INIT_YES | PIC_ICW1_IC4_EXPECT);

    /* Remap IRQs */
    out8(PIC_MASTER_DATA, offset1); /* master offset */
    out8(PIC_SLAVE_DATA, offset2);  /* slave offset */

    /* Tell Master PIC that there is a slave PIC at IRQ2 */
    out8(PIC_MASTER_DATA, 0x04);

    /* Tell Slave PIC its cascade identity */
    out8(PIC_SLAVE_DATA, 0x02);

    /* 8086 mode */
    out8(PIC_MASTER_DATA, PIC_ICW4_UPM_86MODE);
    out8(PIC_SLAVE_DATA, PIC_ICW4_UPM_86MODE);

    /* Restore masks */
    out8(PIC1_REG_IMR, mask1);
    out8(PIC2_REG_IMR, mask2);
}

/* Remap PIC1 to 0x20-0x27 and PIC2 to 0x28-0x2F */
void pic_init(void){pic_remap(0x20, 0x28); }

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8){out8(PIC_SLAVE_CMD, PIC_CMD_EOI);}
    out8(PIC_MASTER_CMD, PIC_CMD_EOI);
}

void pic_mask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8){port = PIC1_REG_IMR;}
    else
    {
        port = PIC2_REG_IMR;
        irq -= 8;
    }

    value = in8(port) | (1 << irq);
    out8(port, value);
}

void pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8){port = PIC1_REG_IMR;}
    else
    {
        port = PIC2_REG_IMR;
        irq -= 8;
    }

    value = in8(port) & ~(1 << irq);
    out8(port, value);
}