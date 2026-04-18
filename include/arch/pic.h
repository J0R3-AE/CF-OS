/**
 * @file pic.h
 * @brief 8259A Programmable Interrupt Controller (PIC) interface.
 *
 * This header defines register ports, initialization command words (ICWs),
 * and helper functions for configuring and controlling the legacy x86 PIC.
 * The PIC is responsible for routing hardware IRQs to the CPU before APIC
 * or IOAPIC takeover.
 */

#ifndef PIC_H
#define PIC_H

#include "libk/types.h"
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* PIC I/O Ports                                                               */
/* -------------------------------------------------------------------------- */

/** @brief Master PIC command port. */
#define PIC_MASTER_CMD 0x20
/** @brief Master PIC data port. */
#define PIC_MASTER_DATA 0x21
/** @brief Slave PIC command port. */
#define PIC_SLAVE_CMD 0xA0
/** @brief Slave PIC data port. */
#define PIC_SLAVE_DATA 0xA1

/** @brief End‑of‑interrupt command. */
#define PIC_CMD_EOI 0x20

/* -------------------------------------------------------------------------- */
/* PIC Register Aliases                                                        */
/* -------------------------------------------------------------------------- */

/* PIC1 */
#define PIC1_REG_COMMAND 0x20
#define PIC1_REG_STATUS 0x20
#define PIC1_REG_DATA 0x21
#define PIC1_REG_IMR 0x21

/* PIC2 */
#define PIC2_REG_COMMAND 0xA0
#define PIC2_REG_STATUS 0xA0
#define PIC2_REG_DATA 0xA1
#define PIC2_REG_IMR 0xA1

/* -------------------------------------------------------------------------- */
/* Initialization Command Word 1 (ICW1)                                        */
/* -------------------------------------------------------------------------- */

#define PIC_ICW1_MASK_IC4 0x01
#define PIC_ICW1_MASK_SNGL 0x02
#define PIC_ICW1_MASK_ADI 0x04
#define PIC_ICW1_MASK_LTIM 0x08
#define PIC_ICW1_MASK_INIT 0x10

/* ICW1 control bits */
#define PIC_ICW1_IC4_EXPECT 0x01
#define PIC_ICW1_IC4_NO 0x00
#define PIC_ICW1_SNGL_YES 0x02
#define PIC_ICW1_SNGL_NO 0x00
#define PIC_ICW1_ADI_CALLINTERVAL4 0x04
#define PIC_ICW1_ADI_CALLINTERVAL8 0x00
#define PIC_ICW1_LTIM_LEVELTRIGGERED 0x08
#define PIC_ICW1_LTIM_EDGETRIGGERED 0x00
#define PIC_ICW1_INIT_YES 0x10
#define PIC_ICW1_INIT_NO 0x00

/* -------------------------------------------------------------------------- */
/* Initialization Command Word 4 (ICW4)                                        */
/* -------------------------------------------------------------------------- */

#define PIC_ICW4_MASK_UPM 0x01
#define PIC_ICW4_MASK_AEOI 0x02
#define PIC_ICW4_MASK_MS 0x04
#define PIC_ICW4_MASK_BUF 0x08
#define PIC_ICW4_MASK_SFNM 0x10

/* ICW4 control bits */
#define PIC_ICW4_UPM_86MODE 0x01
#define PIC_ICW4_UPM_MCSMODE 0x00
#define PIC_ICW4_AEOI_AUTOEOI 0x02
#define PIC_ICW4_AEOI_NOAUTOEOI 0x00
#define PIC_ICW4_MS_BUFFERMASTER 0x04
#define PIC_ICW4_MS_BUFFERSLAVE 0x00
#define PIC_ICW4_BUF_MODEYES 0x08
#define PIC_ICW4_BUF_MODENO 0x00
#define PIC_ICW4_SFNM_NESTEDMODE 0x10
#define PIC_ICW4_SFNM_NOTNESTED 0x00

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize both master and slave PICs.
 *
 * Performs full PIC remapping and configuration using ICW1–ICW4.
 * Typically called early during kernel initialization.
 */
void pic_init(void);

/**
 * @brief Send an End‑Of‑Interrupt (EOI) signal to the PIC.
 *
 * @param irq IRQ line that has been serviced.
 */
void pic_send_eoi(u8 irq);

/**
 * @brief Unmask (enable) a specific IRQ line.
 *
 * @param irq IRQ number to enable.
 */
void pic_unmask_irq(u8 irq);

/**
 * @brief Mask (disable) a specific IRQ line.
 *
 * @param irq IRQ number to disable.
 */
void pic_mask_irq(u8 irq);

/**
 * @brief Remap the PIC interrupt vectors.
 *
 * @param offset1 New vector offset for master PIC.
 * @param offset2 New vector offset for slave PIC.
 */
void pic_remap(int offset1, int offset2);

#endif /* PIC_H */
