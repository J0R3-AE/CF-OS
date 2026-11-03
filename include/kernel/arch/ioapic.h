#pragma once

#include "libc/types.h"

#define IOAPIC_REGISTER_SELECT 0x00u
#define IOAPIC_REGISTER_WINDOW 0x10u
#define IOAPIC_REGISTER_VERSION 0x01u

int ioapic_init(u32 physical_base);
int ioapic_read(u8 register_index, u32 *value_out);
int ioapic_write(u8 register_index, u32 value);
u32 ioapic_max_redirection_entry(void);
