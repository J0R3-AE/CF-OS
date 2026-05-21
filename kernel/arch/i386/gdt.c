#include "arch/gdt.h"
#include "libk/string.h"
#include "libk/printf.h"
#include "libk/log.h"

/* GDT entries array - 6 entries: null, kernel code, kernel data, user code, user data, TSS */
gdt_entry_t gdt_entries[6];
gdt_ptr_t gdt_ptr;

/* External assembly function to load the GDT */
extern void i386GDT_flush(u32);

extern uintptr_t tss_base;
extern uint32_t tss_limit;

/*
 * Set a GDT entry with the specified parameters
 *
 * Parameters:
 *   num - Index in the GDT array
 *   base - Base address of the segment
 *   limit - Size of the segment
 *   access - Access flags (privilege level, segment type, etc.)
 *   gran - Granularity flags
 */

void gdt_set_gate(s32 num, u32 base, u32 limit, u8 access, u8 gran)
{
    /* Set up the base address */
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    /* Set up the limit */
    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    /* Set up granularity and access flags */
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access = access;
}

/*
 * Initialize the Global Descriptor Table
 *
 * The GDT defines memory segments for the processor:
 * - Entry 0: Null segment (required by x86)
 * - Entry 1: Kernel code segment (ring 0, executable)
 * - Entry 2: Kernel data segment (ring 0, writable)
 * - Entry 3: User code segment (ring 3, executable)
 * - Entry 4: User data segment (ring 3, writable)
 */
void gdt_init(void)
{
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base = (u32)&gdt_entries;

    /* Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Kernel code segment
     * Base: 0x00000000, Limit: 0xFFFFFFFF
     * Access: 0x9A (Present, Ring 0, Code, Executable, Readable)
     * Granularity: 0xCF (4KB granularity, 32-bit)
     */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    /* Kernel data segment
     * Base: 0x00000000, Limit: 0xFFFFFFFF
     * Access: 0x92 (Present, Ring 0, Data, Writable)
     * Granularity: 0xCF (4KB granularity, 32-bit)
     */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    /* User code segment
     * Base: 0x00000000, Limit: 0xFFFFFFFF
     * Access: 0xFA (Present, Ring 3, Code, Executable, Readable)
     * Granularity: 0xCF (4KB granularity, 32-bit)
     */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    /* User data segment
     * Base: 0x00000000, Limit: 0xFFFFFFFF
     * Access: 0xF2 (Present, Ring 3, Data, Writable)
     * Granularity: 0xCF (4KB granularity, 32-bit)
     */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    /* Load the new GDT */
    i386GDT_flush((u32)&gdt_ptr);
}
