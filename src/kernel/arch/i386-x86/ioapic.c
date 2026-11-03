#include "kernel/arch/apic.h"
#include "kernel/arch/ioapic.h"

static volatile u32 *ioapic_registers;
static u32 ioapic_max_entry;

static void ioapic_select(u8 index)
{
    ioapic_registers[IOAPIC_REGISTER_SELECT / sizeof(u32)] = index;
}

int ioapic_init(u32 physical_base)
{
    u32 version;

    if (physical_base & 0xFFFu || apic_map_mmio(physical_base) != 0)
        return -1;

    ioapic_registers = (volatile u32 *)physical_base;
    if (ioapic_read(IOAPIC_REGISTER_VERSION, &version) != 0)
    {
        ioapic_registers = 0;
        return -1;
    }
    ioapic_max_entry = (version >> 16) & 0xFFu;
    if (!version)
    {
        ioapic_registers = 0;
        ioapic_max_entry = 0;
        return -1;
    }

    return 0;
}

int ioapic_read(u8 register_index, u32 *value_out)
{
    if (!ioapic_registers || !value_out)
        return -1;

    ioapic_select(register_index);
    *value_out = ioapic_registers[IOAPIC_REGISTER_WINDOW / sizeof(u32)];
    return 0;
}

int ioapic_write(u8 register_index, u32 value)
{
    if (!ioapic_registers)
        return -1;

    ioapic_select(register_index);
    ioapic_registers[IOAPIC_REGISTER_WINDOW / sizeof(u32)] = value;
    return 0;
}

u32 ioapic_max_redirection_entry(void)
{
    return ioapic_max_entry;
}