/**
 * @file io.h
 * @brief Low‑level hardware I/O helpers for x86 (port I/O, MMIO, CPU control).
 *
 * This header provides wrappers around x86 port‑mapped I/O instructions,
 * memory‑mapped I/O accessors, interrupt control helpers, and basic serial
 * output routines. These functions form the foundation of device drivers
 * and early boot diagnostics.
 */

#pragma once
#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* Port‑Mapped I/O (PIO)                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read an 8‑bit value from an I/O port.
 *
 * @param port I/O port number.
 * @return 8‑bit value read.
 */
u8 io_Read8(u16 port);

/**
 * @brief Read a 16‑bit value from an I/O port.
 */
u16 io_Read16(u16 port);

/**
 * @brief Read a 32‑bit value from an I/O port.
 */
u32 io_Read32(u16 port);

/**
 * @brief Write an 8‑bit value to an I/O port.
 */
void io_Write8(u16 port, u8 data);

/**
 * @brief Write a 16‑bit value to an I/O port.
 */
void io_Write16(u16 port, u16 data);

/**
 * @brief Write a 32‑bit value to an I/O port.
 */
void io_Write32(u16 port, u32 data);

/* -------------------------------------------------------------------------- */
/* Memory‑Mapped I/O (MMIO)                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read an 8‑bit value from a memory‑mapped I/O address.
 */
u8 mmio_Read8(void *p);

/**
 * @brief Read a 16‑bit value from a memory‑mapped I/O address.
 */
u16 mmio_Read16(void *p);

/**
 * @brief Read a 32‑bit value from a memory‑mapped I/O address.
 */
u32 mmio_Read32(void *p);

/**
 * @brief Write an 8‑bit value to a memory‑mapped I/O address.
 */
void mmio_Write8(void *p, u8 data);

/**
 * @brief Write a 16‑bit value to a memory‑mapped I/O address.
 */
void mmio_Write16(void *p, u16 data);

/**
 * @brief Write a 32‑bit value to a memory‑mapped I/O address.
 */
void mmio_Write32(void *p, u32 data);

/* -------------------------------------------------------------------------- */
/* Raw x86 Port I/O (ASM versions)                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read an 8‑bit value from an I/O port (inline assembly).
 */
u8 i386io_inb(u16 port);

/**
 * @brief Read a 16‑bit value from an I/O port (inline assembly).
 */
u16 i386io_inw(u16 port);

/**
 * @brief Read a 32‑bit value from an I/O port (inline assembly).
 */
u32 i386io_inl(u16 port);

/**
 * @brief Write an 8‑bit value to an I/O port (inline assembly).
 */
void i386io_outb(u16 port, u8 value);

/**
 * @brief Write a 16‑bit value to an I/O port (inline assembly).
 */
void i386io_outw(u16 port, u16 value);

/**
 * @brief Write a 32‑bit value to an I/O port (inline assembly).
 */
void i386io_outl(u16 port, u32 value);

/* -------------------------------------------------------------------------- */
/* CPU Interrupt Control & Halt                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Enable CPU interrupts (set IF flag).
 */
void io_enableinterrupts(void);

/**
 * @brief Disable CPU interrupts (clear IF flag).
 */
void io_disableinterrupts(void);

/**
 * @brief Halt the CPU until the next external interrupt.
 */
void io_halt(void);

/**
 * @brief Enable CPU interrupts (assembly version).
 */
void i386io_enableinterrupts(void);

/**
 * @brief Disable CPU interrupts (assembly version).
 */
void i386io_disableinterrupts(void);

/**
 * @brief Halt the CPU until the next interrupt (assembly version).
 */
void i386io_hlt(void);

/**
 * @brief Delay briefly to allow slow I/O devices to settle.
 *
 * Typically implemented as an outb to port 0x80.
 */
void i386io_iowait(void);

/**
 * @brief Print a panic message and halt the system.
 *
 * @param msg Null‑terminated panic message.
 */
void i386io_panic(const char *msg);

/**
 * @brief Standard I/O wait helper (alias for iowait).
 */
void io_wait(void);

/* -------------------------------------------------------------------------- */
/* Serial Port Output (COM1)                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the serial port for debugging output.
 */
void i386SERIAL_init(void);

/**
 * @brief Write a single character to the serial port.
 */
void i386SERIAL_write(char c);

/**
 * @brief Write a null‑terminated string to the serial port.
 */
void i386SERIAL_writestr(const char *s);

/* IO_H */
