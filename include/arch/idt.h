/**
 * @file idt.h
 * @brief x86 Interrupt Descriptor Table (IDT) structures and API.
 *
 * This header defines the 32‑bit IDT entry format, the IDT pointer used
 * by the `lidt` instruction, and the CPU register frame pushed on the
 * stack during an interrupt or exception. It also exposes functions for
 * initializing the IDT and registering interrupt service routines (ISRs).
 */

#ifndef IDT_H
#define IDT_H

#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* IDT Entry Structure                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @struct idt_entry
 * @brief 32‑bit IDT gate descriptor.
 *
 * Represents an interrupt or trap gate in the IDT. The handler address
 * is split into two 16‑bit halves. The `flags` field encodes privilege
 * level, present bit, and gate type.
 */
typedef struct idt_entry
{
    u16 base_low;  /**< Lower 16 bits of ISR handler address. */
    u16 selector;  /**< Kernel code segment selector. */
    u8 always0;    /**< Must be zero. */
    u8 flags;      /**< Type, DPL, and present bit. */
    u16 base_high; /**< Upper 16 bits of ISR handler address. */
} __attribute__((packed)) idt_entry_t;

/* -------------------------------------------------------------------------- */
/* IDT Pointer                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @struct idt_ptr
 * @brief Pointer structure used by the `lidt` instruction.
 */
typedef struct idt_ptr
{
    u16 limit; /**< Size of IDT in bytes minus one. */
    u32 base;  /**< Linear address of the first IDT entry. */
} __attribute__((packed)) idt_ptr_t;

/* -------------------------------------------------------------------------- */
/* CPU Register Frame                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @struct registers
 * @brief CPU state pushed by the processor and ISR stubs.
 *
 * This structure represents the full interrupt frame pushed onto the
 * stack during an interrupt or exception. It includes segment registers,
 * general‑purpose registers, interrupt number, error code (if any), and
 * the saved execution context.
 */
typedef struct registers
{
    u32 gs, fs, es, ds;               /**< Segment registers. */
    u32 edi, esi, ebp, esp;           /**< General‑purpose registers. */
    u32 ebx, edx, ecx, eax;           /**< General‑purpose registers. */
    u32 int_no;                       /**< Interrupt number. */
    u32 err_code;                     /**< Error code (or zero). */
    u32 eip, cs, eflags, useresp, ss; /**< CPU‑pushed state. */
} registers_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the Interrupt Descriptor Table.
 *
 * Sets up all IDT entries, loads the IDT pointer with `lidt`, and
 * installs default ISR/IRQ stubs.
 */
void idt_init(void);

/**
 * @brief Function pointer type for interrupt handlers.
 *
 * @param regs Pointer to the CPU register frame.
 */
typedef void (*isr_handler_t)(registers_t *regs);

/**
 * @brief Register a custom interrupt handler.
 *
 * @param n Interrupt vector number (0–255).
 * @param handler Function to call when the interrupt occurs.
 */
void register_interrupt_handler(u8 n, isr_handler_t handler);

#endif /* IDT_H */
