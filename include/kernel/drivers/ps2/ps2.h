#ifndef PS2_H
#define PS2_H

#include <stdint.h>
#include <kernel/arch/idt.h>

void ps2_init(void);
void ps2_irq_keyboard(registers_t *regs);
void ps2_register_keyboard_callback(void (*cb)(uint8_t));

#endif
