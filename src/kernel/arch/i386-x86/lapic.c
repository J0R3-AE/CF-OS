#include "kernel/arch/apic.h"
#include "kernel/arch/lapic.h"

#define LAPIC_REGISTER_LIMIT 0x3F0u

static volatile u32 *lapic_registers;

int lapic_init(u32 physical_base)
{
    if (physical_base & 0xFFFu || apic_map_mmio(physical_base) != 0)
        return -1;

    lapic_registers = (volatile u32 *)physical_base;
    if (lapic_write(LAPIC_REGISTER_TPR, 0) != 0)
        return -1;

    return lapic_write(LAPIC_REGISTER_SVR,
                       (1u << 8) | LAPIC_SPURIOUS_VECTOR);
}

int lapic_read(u32 offset, u32 *value_out)
{
    if (!lapic_registers || !value_out || (offset & 0xFu) ||
        offset > LAPIC_REGISTER_LIMIT)
        return -1;

    *value_out = lapic_registers[offset / sizeof(u32)];
    return 0;
}

int lapic_write(u32 offset, u32 value)
{
    if (!lapic_registers || (offset & 0xFu) || offset > LAPIC_REGISTER_LIMIT)
        return -1;

    lapic_registers[offset / sizeof(u32)] = value;
    return 0;
}

void lapic_eoi(void)
{
    if (lapic_registers)
        lapic_registers[LAPIC_REGISTER_EOI / sizeof(u32)] = 0;
}