/**
 * @file tss.h
 * @brief x86 Task State Segment (TSS) structure and exported symbols.
 *
 * The TSS is used by the x86 CPU to perform privilege‑level transitions,
 * most importantly switching to a kernel stack (esp0/ss0) when entering
 * ring 0 from user mode. This header defines the packed TSS layout used
 * by the kernel along with extern references to the active TSS instance
 * and its descriptor metadata.
 */

#pragma once

#include <stdint.h>

/**
 * @struct tss_entry
 * @brief Hardware-defined x86 Task State Segment (TSS).
 *
 * This structure matches the 32‑bit TSS layout required by the CPU.
 * Only a subset of fields are used by modern kernels; most multitasking
 * is handled manually rather than via hardware task switching.
 *
 * The most important fields are:
 *  - esp0 / ss0 : kernel stack pointer used on privilege transitions
 *  - iomap_base : offset to the I/O permission bitmap
 */
struct tss_entry {
    uint32_t prev_tss;     /**< Previous TSS (unused in software multitasking). */
    uint32_t esp0;         /**< Kernel stack pointer for ring 0 transitions. */
    uint32_t ss0;          /**< Kernel stack segment for ring 0. */
    uint32_t esp1;         /**< Ring 1 stack pointer (unused). */
    uint32_t ss1;          /**< Ring 1 stack segment (unused). */
    uint32_t esp2;         /**< Ring 2 stack pointer (unused). */
    uint32_t ss2;          /**< Ring 2 stack segment (unused). */
    uint32_t cr3;          /**< Page directory base (optional). */
    uint32_t eip;          /**< Saved instruction pointer (unused). */
    uint32_t eflags;       /**< Saved flags (unused). */
    uint32_t eax, ecx, edx, ebx; /**< General-purpose registers (unused). */
    uint32_t esp, ebp, esi, edi; /**< More general-purpose registers. */
    uint32_t es, cs, ss, ds, fs, gs; /**< Segment selectors. */
    uint32_t ldt;          /**< Local Descriptor Table selector. */
    uint16_t trap;         /**< Trap flag (unused). */
    uint16_t iomap_base;   /**< Offset to I/O permission bitmap. */
} __attribute__((packed));

/**
 * @brief Active TSS instance used by the kernel.
 *
 * Defined in the architecture-specific initialization code.
 */
extern struct tss_entry tss;

/**
 * @brief Base address of the TSS in memory (for GDT descriptor setup).
 */
extern uintptr_t tss_base;

/**
 * @brief Size of the TSS structure (limit field for GDT descriptor).
 */
extern uint32_t tss_limit;

void tss_init(uint32_t kernel_stack_top);
