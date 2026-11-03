#pragma once

#include "libc/types.h"

#define LAPIC_REGISTER_ID 0x020u
#define LAPIC_REGISTER_TPR 0x080u
#define LAPIC_REGISTER_EOI 0x0B0u
#define LAPIC_REGISTER_SVR 0x0F0u
#define LAPIC_SPURIOUS_VECTOR 0xFFu

int lapic_init(u32 physical_base);
int lapic_read(u32 offset, u32 *value_out);
int lapic_write(u32 offset, u32 value);
void lapic_eoi(void);
