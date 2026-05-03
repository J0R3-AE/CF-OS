#include "arch/tss.h"
#include "arch/gdt.h"
#include "libk/log.h"
#include "libk/mem.h"

extern void i386GDT_flush(u32);
extern void tss_flush(uint16_t sel);
extern void gdt_set_gate(s32 num, u32 base, u32 limit, u8 access, u8 gran);

extern gdt_entry_t gdt_entries[];
extern gdt_ptr_t   gdt_ptr;

struct tss_entry tss;
uintptr_t tss_base  = (uintptr_t)&tss;
uint32_t tss_limit  = sizeof(tss) - 1;

void tss_init(uint32_t kernel_stack_top)
{
    memset(&tss, 0, sizeof(tss));

    tss.ss0  = 0x10;             // kernel data
    tss.esp0 = kernel_stack_top;
    tss.iomap_base = sizeof(tss);

    // put TSS in GDT entry 5
    gdt_set_gate(5,
                 (u32)tss_base,
                 tss_limit,
                 0x89,  // present, DPL=0, 32‑bit available TSS
                 0x00); // byte granularity

    // GDT already loaded; no need to flush again

    i386GDT_flush((u32)&gdt_ptr);
    tss_flush(0x28); // selector = 5 << 3
    klog_log("Tss: esp0=%x ss0=%x", tss.esp0, tss.ss0);
}
