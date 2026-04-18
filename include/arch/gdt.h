/**
 * @file gdt.h
 * @brief x86 Global Descriptor Table (GDT) structures and initialization API.
 *
 * The GDT defines memory segments used by the CPU for privilege separation,
 * code/data access, and task switching. Modern kernels typically use a flat
 * memory model, but still require a valid GDT for protected mode operation.
 */

#ifndef GDT_H
#define GDT_H

#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* GDT Entry Structure                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @struct gdt_entry
 * @brief 32‑bit GDT descriptor entry.
 *
 * A GDT entry describes a memory segment. The base and limit fields are
 * split across multiple parts to match the hardware format. The `access`
 * and `granularity` fields encode privilege level, segment type, and size.
 */
typedef struct gdt_entry
{
    u16 limit_low;  /**< Lower 16 bits of segment limit. */
    u16 base_low;   /**< Lower 16 bits of segment base. */
    u8 base_middle; /**< Middle 8 bits of segment base. */
    u8 access;      /**< Access flags (present, ring, type). */
    u8 granularity; /**< Granularity + upper 4 bits of limit. */
    u8 base_high;   /**< Upper 8 bits of segment base. */
} __attribute__((packed)) gdt_entry_t;

/* -------------------------------------------------------------------------- */
/* GDT Pointer                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @struct gdt_ptr
 * @brief Pointer structure used by the `lgdt` instruction.
 *
 * The CPU loads this structure to install a new GDT.
 */
typedef struct gdt_ptr
{
    u16 limit; /**< Size of GDT in bytes minus one. */
    u32 base;  /**< Linear address of the first GDT entry. */
} __attribute__((packed)) gdt_ptr_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the Global Descriptor Table.
 *
 * Sets up kernel code/data segments, optionally user segments, and loads
 * the GDT using the `lgdt` instruction. Also reloads segment registers.
 */
void gdt_init(void);

/**
 * @brief Set a GDT entry.
 *
 * @param num     Index of the GDT entry to modify.
 * @param base    Base address of the segment.
 * @param limit   Segment limit.
 * @param access  Access flags (type, privilege, present).
 * @param gran    Granularity flags (size, granularity).
 */
void gdt_set_gate(s32 num, u32 base, u32 limit, u8 access, u8 gran);

#endif /* GDT_H */
