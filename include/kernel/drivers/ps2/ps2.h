#ifndef PS2_H
#define PS2_H

#include "libc/types.h"
#include "kernel/arch/idt.h"

int ps2_init(void);
void ps2_irq_handler(registers_t *regs);
void ps2_register_keyboard_callback(void (*callback)(u8));

#endif
