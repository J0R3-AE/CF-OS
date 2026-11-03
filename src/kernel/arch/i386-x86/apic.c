#include "kernel/arch/apic.h"
#include "kernel/arch/lapic.h"
#include "kernel/mm/paging.h"

static bool apic_has_cpuid_feature(u32 leaf, u32 register_mask)
{
    u32 eax = leaf;
    u32 ebx;
    u32 ecx;
    u32 edx;

    __asm__ volatile("cpuid"
                     : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
    (void)ebx;
    (void)ecx;
    return (edx & register_mask) != 0;
}

bool apic_supported(void)
{
    return apic_has_cpuid_feature(1, 1u << 9) &&
           apic_has_cpuid_feature(1, 1u << 5);
}

int apic_map_mmio(u32 physical_address)
{
    if (!kernel_pd || (physical_address & 0xFFFu))
        return -1;

    return paging_map_page(kernel_pd, physical_address, physical_address,
                           PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
}

int apic_init(void)
{
    u32 low;
    u32 high;
    u32 physical_base;

    if (!apic_supported() || !kernel_pd)
        return -1;

    __asm__ volatile("rdmsr"
                     : "=a"(low), "=d"(high)
                     : "c"(APIC_MSR_BASE));
    (void)high;

    if (low & APIC_MSR_X2APIC)
        return -1;

    physical_base = low & APIC_PHYSICAL_BASE_MASK;
    if (!physical_base || apic_map_mmio(physical_base) != 0)
        return -1;

    if (!(low & APIC_MSR_ENABLE))
    {
        low |= APIC_MSR_ENABLE;
        __asm__ volatile("wrmsr"
                         :
                         : "a"(low), "d"(high), "c"(APIC_MSR_BASE)
                         : "memory");
    }

    return lapic_init(physical_base);
}