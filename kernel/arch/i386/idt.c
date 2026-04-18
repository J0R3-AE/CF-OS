#include "arch/idt.h"
#include "arch/io.h"
#include "arch/pic.h"

#include "libk/string.h"
#include "libk/mem.h"
#include "libk/types.h"

#include "libk/log.h"

/* IDT entries array - 256 entries for all possible interrupts */
static idt_entry_t idt_entries[256];
static idt_ptr_t idt_ptr;

/* Interrupt handler table */
static isr_handler_t interrupt_handlers[256];

/* External assembly function to load the IDT */
extern void i386IDT_flush(u32);

/* External ISR handler declarations (defined in isr.S) */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void isr128(void);

/* IRQ handlers (hardware interrupts) */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/*
 * Set an IDT entry with the specified parameters
 *
 * Parameters:
 *   num - Index in the IDT array
 *   base - Address of the interrupt handler function
 *   selector - Kernel code segment selector (usually 0x08)
 *   flags - Type and privilege level flags
 */
static void idt_set_gate(u8 num, u32 base, u16 selector, u8 flags)
{
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].selector = selector;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags = flags;
    klog_info("IDT: Set gate %u base=%x selector=%x flags=%x", num, base, selector, flags);
}

/*
 * Initialize the Interrupt Descriptor Table
 *
 * Sets up all 256 possible interrupt entries:
 * - 0-31: CPU exceptions (divide by zero, page fault, etc.)
 * - 32-47: Hardware interrupts (remapped from IRQ 0-15)
 * - 48-255: Available for software interrupts
 */
void idt_init(void)
{
    idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_ptr.base = (u32)&idt_entries;

    /* Clear all entries */
    memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);
    memset(&interrupt_handlers, 0, sizeof(isr_handler_t) * 256);

    /* Install ISRs for CPU exceptions (0-31) */
    /* Flags: 0x8E = Present, Ring 0, 32-bit Interrupt Gate */
    idt_set_gate(0, (u32)isr0, 0x08, 0x8E);
    idt_set_gate(1, (u32)isr1, 0x08, 0x8E);
    idt_set_gate(2, (u32)isr2, 0x08, 0x8E);
    idt_set_gate(3, (u32)isr3, 0x08, 0x8E);
    idt_set_gate(4, (u32)isr4, 0x08, 0x8E);
    idt_set_gate(5, (u32)isr5, 0x08, 0x8E);
    idt_set_gate(6, (u32)isr6, 0x08, 0x8E);
    idt_set_gate(7, (u32)isr7, 0x08, 0x8E);
    idt_set_gate(8, (u32)isr8, 0x08, 0x8E);
    idt_set_gate(9, (u32)isr9, 0x08, 0x8E);
    idt_set_gate(10, (u32)isr10, 0x08, 0x8E);
    idt_set_gate(11, (u32)isr11, 0x08, 0x8E);
    idt_set_gate(12, (u32)isr12, 0x08, 0x8E);
    idt_set_gate(13, (u32)isr13, 0x08, 0x8E);
    idt_set_gate(14, (u32)isr14, 0x08, 0x8E);
    idt_set_gate(15, (u32)isr15, 0x08, 0x8E);
    idt_set_gate(16, (u32)isr16, 0x08, 0x8E);
    idt_set_gate(17, (u32)isr17, 0x08, 0x8E);
    idt_set_gate(18, (u32)isr18, 0x08, 0x8E);
    idt_set_gate(19, (u32)isr19, 0x08, 0x8E);
    idt_set_gate(20, (u32)isr20, 0x08, 0x8E);
    idt_set_gate(21, (u32)isr21, 0x08, 0x8E);
    idt_set_gate(22, (u32)isr22, 0x08, 0x8E);
    idt_set_gate(23, (u32)isr23, 0x08, 0x8E);
    idt_set_gate(24, (u32)isr24, 0x08, 0x8E);
    idt_set_gate(25, (u32)isr25, 0x08, 0x8E);
    idt_set_gate(26, (u32)isr26, 0x08, 0x8E);
    idt_set_gate(27, (u32)isr27, 0x08, 0x8E);
    idt_set_gate(28, (u32)isr28, 0x08, 0x8E);
    idt_set_gate(29, (u32)isr29, 0x08, 0x8E);
    idt_set_gate(30, (u32)isr30, 0x08, 0x8E);
    idt_set_gate(31, (u32)isr31, 0x08, 0x8E);

    /* Install IRQs (hardware interrupts 32-47) */
    idt_set_gate(32, (u32)irq0, 0x08, 0x8E);
    idt_set_gate(33, (u32)irq1, 0x08, 0x8E);
    idt_set_gate(34, (u32)irq2, 0x08, 0x8E);
    idt_set_gate(35, (u32)irq3, 0x08, 0x8E);
    idt_set_gate(36, (u32)irq4, 0x08, 0x8E);
    idt_set_gate(37, (u32)irq5, 0x08, 0x8E);
    idt_set_gate(38, (u32)irq6, 0x08, 0x8E);
    idt_set_gate(39, (u32)irq7, 0x08, 0x8E);
    idt_set_gate(40, (u32)irq8, 0x08, 0x8E);
    idt_set_gate(41, (u32)irq9, 0x08, 0x8E);
    idt_set_gate(42, (u32)irq10, 0x08, 0x8E);
    idt_set_gate(43, (u32)irq11, 0x08, 0x8E);
    idt_set_gate(44, (u32)irq12, 0x08, 0x8E);
    idt_set_gate(45, (u32)irq13, 0x08, 0x8E);
    idt_set_gate(46, (u32)irq14, 0x08, 0x8E);
    idt_set_gate(47, (u32)(irq15), 0x08, 0x8E);

    /* Load the IDT */
    i386IDT_flush((u32)&idt_ptr);

    // test_syscall_from_kernel();
}

/*
 * Register a custom handler for a specific interrupt
 *
 * Parameters:
 *   n - Interrupt number (0-255)
 *   handler - Function pointer to the handler
 */
void register_interrupt_handler(u8 n, isr_handler_t handler)
{
    interrupt_handlers[n] = handler;
    klog_info("Registered handler for interrupt %u at %p", n, (void *)handler);
}

/*
 * Common ISR handler called from assembly
 * Dispatches to registered handlers
 */
void isr_handler(registers_t *regs)
{

    if (interrupt_handlers[regs->int_no])
    {
        interrupt_handlers[regs->int_no](regs);
    }
    klog_warn("ISR: Interrupt %u occurred with no handler", regs->int_no);
}

/*
 * Common IRQ handler called from assembly
 * Sends End-Of-Interrupt signal to PIC and dispatches to handlers
 */
void irq_handler(registers_t *regs)
{
    // Send EOI to PIC
    if (regs->int_no >= 40)
    {
        io_Write8(PIC_SLAVE_CMD, PIC_CMD_EOI);
    }
    io_Write8(PIC_MASTER_CMD, PIC_CMD_EOI);

    if (regs->int_no < 256 && interrupt_handlers[regs->int_no])
    {
        interrupt_handlers[regs->int_no](regs);
    }
    else
    {
        klog_warn("IRQ: Interrupt %u occurred with no handler", regs->int_no);
    }
}

void page_fault_handler(registers_t *r)
{
    uint32_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    klog_misc("PAGE FAULT: cr2=%x err=%x eip=%x\n",
              cr2, r->err_code, r->eip);

    // optionally halt so it doesn't spin
    for (;;)
        ;
}