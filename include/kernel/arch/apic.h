#pragma once

#include "libc/types.h"

#define APIC_MSR_BASE 0x1Bu
#define APIC_MSR_ENABLE (1u << 11)
#define APIC_MSR_X2APIC (1u << 10)
#define APIC_PHYSICAL_BASE_MASK 0xFFFFF000u

bool apic_supported(void);
int apic_init(void);
int apic_map_mmio(u32 physical_address);
