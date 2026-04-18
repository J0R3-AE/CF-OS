/**
 * @file pit.h
 * @brief Programmable Interval Timer (PIT) driver for x86 (8253/8254).
 *
 * This header defines register constants, mode flags, and the public API
 * for configuring and handling the PIT. The PIT is used to generate
 * periodic interrupts (IRQ0), which drive the kernel scheduler tick and
 * global timekeeping.
 */

#pragma once

#include <stdint.h>
#include "arch/idt.h"
#include "libk/types.h"
#include "sched/sched.h"

/* -------------------------------------------------------------------------- */
/* I/O Port Definitions                                                        */
/* -------------------------------------------------------------------------- */

/** @brief Channel 0 data port (system timer). */
#define PIT_REG_COUNTER0 0x40
/** @brief Channel 1 data port (DRAM refresh, rarely used). */
#define PIT_REG_COUNTER1 0x41
/** @brief Channel 2 data port (PC speaker). */
#define PIT_REG_COUNTER2 0x42
/** @brief PIT command register. */
#define PIT_REG_COMMAND 0x43

/* -------------------------------------------------------------------------- */
/* Operational Command Word (OCW) Masks                                        */
/* -------------------------------------------------------------------------- */

#define PIT_OCW_MASK_BINCOUNT 0x01 /**< Binary or BCD mode. */
#define PIT_OCW_MASK_MODE 0x0E     /**< Operating mode bits. */
#define PIT_OCW_MASK_RL 0x30       /**< Read/load format. */
#define PIT_OCW_MASK_COUNTER 0xC0  /**< Counter select. */

/* -------------------------------------------------------------------------- */
/* Binary/BCD Mode                                                             */
/* -------------------------------------------------------------------------- */

#define PIT_OCW_BINCOUNT_BINARY 0x00 /**< Use binary counting. */
#define PIT_OCW_BINCOUNT_BCD 0x01    /**< Use BCD counting (rare). */

/* -------------------------------------------------------------------------- */
/* Operating Modes                                                             */
/* -------------------------------------------------------------------------- */

#define PIT_OCW_MODE_TERMINALCOUNT 0x00 /**< Interrupt on terminal count. */
#define PIT_OCW_MODE_ONESHOT 0x02       /**< One-shot mode. */
#define PIT_OCW_MODE_RATEGEN 0x04       /**< Rate generator. */
#define PIT_OCW_MODE_SQUAREWAVEGEN 0x06 /**< Square wave generator (common). */
#define PIT_OCW_MODE_SOFTWARETRIG 0x08  /**< Software triggered strobe. */
#define PIT_OCW_MODE_HARDWARETRIG 0x0A  /**< Hardware triggered strobe. */

/* -------------------------------------------------------------------------- */
/* Read/Load (RL) Modes                                                        */
/* -------------------------------------------------------------------------- */

#define PIT_OCW_RL_LATCH 0x00   /**< Latch count value. */
#define PIT_OCW_RL_LSBONLY 0x10 /**< Load/read LSB only. */
#define PIT_OCW_RL_MSBONLY 0x20 /**< Load/read MSB only. */
#define PIT_OCW_RL_DATA 0x30    /**< Load/read LSB then MSB. */

/* -------------------------------------------------------------------------- */
/* Counter Select                                                              */
/* -------------------------------------------------------------------------- */

#define PIT_OCW_COUNTER_0 0x00 /**< Select counter 0. */
#define PIT_OCW_COUNTER_1 0x40 /**< Select counter 1. */
#define PIT_OCW_COUNTER_2 0x80 /**< Select counter 2. */

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the PIT to generate periodic interrupts.
 *
 * Configures PIT channel 0 to fire IRQ0 at the specified frequency.
 *
 * @param hz Desired interrupt frequency in Hz.
 */
void pit_init(u32 hz);

/**
 * @brief PIT interrupt handler (IRQ0).
 *
 * Called from the IDT dispatcher. Updates kernel tick counters and
 * triggers scheduler activity.
 *
 * @param r Pointer to saved CPU register state.
 */
void pit_handler(registers_t *r);

/**
 * @brief Get the current tick count since PIT initialization.
 *
 * @return Number of ticks elapsed.
 */
u32 pit_get_ticks(void);

/**
 * @brief Get the current PIT frequency.
 *
 * @return Frequency in Hz.
 */
u32 pit_get_frequency(void);

/**
 * @brief Reset the global tick counter to zero.
 */
void pit_reset_ticks(void);

/* -------------------------------------------------------------------------- */
/* Internal Helpers (not exposed publicly)                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Program PIT channel 0 to the specified frequency.
 *
 * Internal helper used by pit_init().
 *
 * @param hz Desired frequency.
 */
static void pit_program(u32 hz);

/* __PIT_H_ */
