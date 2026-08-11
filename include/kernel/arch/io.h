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
#include "libc/types.h"

/* -------------------------------------------------------------------------- */
/* Port‑Mapped I/O (PIO)                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read an 8‑bit value from an I/O port.
 *
 * @param port I/O port number.
 * @return 8‑bit value read.
 */
u8 in8(u16 port);

/**
 * @brief Read a 16‑bit value from an I/O port.
 */
u16 in16(u16 port);

/**
 * @brief Read a 32‑bit value from an I/O port.
 */
u32 in32(u16 port);

/**
 * @brief Write an 8‑bit value to an I/O port.
 */
void out8(u16 port, u8 data);

/**
 * @brief Write a 16‑bit value to an I/O port.
 */
void out16(u16 port, u16 data);

/**
 * @brief Write a 32‑bit value to an I/O port.
 */
void out32(u16 port, u32 data);

/* -------------------------------------------------------------------------- */
/* Memory‑Mapped I/O (MMIO)                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read an 8‑bit value from a memory‑mapped I/O address.
 */
u8 mmin8(void *p);

/**
 * @brief Read a 16‑bit value from a memory‑mapped I/O address.
 */
u16 mmin16(void *p);

/**
 * @brief Read a 32‑bit value from a memory‑mapped I/O address.
 */
u32 mmin32(void *p);

/**
 * @brief Write an 8‑bit value to a memory‑mapped I/O address.
 */
void mmout8(void *p, u8 data);

/**
 * @brief Write a 16‑bit value to a memory‑mapped I/O address.
 */
void mmout16(void *p, u16 data);

/**
 * @brief Write a 32‑bit value to a memory‑mapped I/O address.
 */
void mmout32(void *p, u32 data);

/* -------------------------------------------------------------------------- */
/* CPU Interrupt Control & Halt                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Enable CPU interrupts (set IF flag).
 */
void enableinterrupts(void);

/**
 * @brief Disable CPU interrupts (clear IF flag).
 */
void disableinterrupts(void);

/**
 * @brief Halt the CPU until the next external interrupt.
 */
void halt(void);

/**
 * @brief Standard I/O wait helper (alias for iowait).
 */
void wait(void);

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
