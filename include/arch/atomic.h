#pragma once
/**
 * @file atomic.h
 * @brief Basic atomic operations for 32‑bit x86.
 *
 * Provides atomic exchange, compare‑and‑swap, fetch‑add/sub, and ordered
 * load/store primitives. These operations are implemented using x86
 * `lock`‑prefixed instructions to guarantee atomicity across CPUs.
 */

#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/*  Atomic Operations API                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Atomically exchange the value at @p ptr with @p val.
 *
 * @param ptr Pointer to the target variable.
 * @param val New value to store.
 * @return The previous value stored at @p ptr.
 */
u32 atomic_exchange(volatile u32 *ptr, u32 val);

/**
 * @brief Compare‑and‑swap (CAS).
 *
 * Atomically compares `*ptr` with @p expected.
 * If equal, writes @p desired into `*ptr`.
 *
 * @param ptr      Pointer to the target variable.
 * @param expected Expected old value.
 * @param desired  New value to store if comparison succeeds.
 * @return The previous value of `*ptr` (regardless of success).
 */
u32 atomic_cmpxchg(volatile u32 *ptr, u32 expected, u32 desired);

/**
 * @brief Atomically add @p inc to `*ptr`.
 *
 * @param ptr Pointer to the target variable.
 * @param inc Amount to add.
 * @return The previous value of `*ptr`.
 */
u32 atomic_fetch_add(volatile u32 *ptr, u32 inc);

/**
 * @brief Atomically subtract @p dec from `*ptr`.
 *
 * @param ptr Pointer to the target variable.
 * @param dec Amount to subtract.
 * @return The previous value of `*ptr`.
 */
u32 atomic_fetch_sub(volatile u32 *ptr, u32 dec);

/**
 * @brief Atomically add @p inc to `*ptr` (discarding the old value).
 */
void atomic_add(volatile u32 *ptr, u32 inc);

/**
 * @brief Atomically subtract @p dec from `*ptr` (discarding the old value).
 */
void atomic_sub(volatile u32 *ptr, u32 dec);

/**
 * @brief Ordered atomic load.
 *
 * Ensures the load is not reordered across memory barriers.
 *
 * @param ptr Pointer to the value to load.
 * @return The loaded value.
 */
u32 atomic_load(volatile u32 *ptr);

/**
 * @brief Ordered atomic store.
 *
 * Ensures the store is not reordered across memory barriers.
 *
 * @param ptr Pointer to the target variable.
 * @param val Value to store.
 */
void atomic_store(volatile u32 *ptr, u32 val);

/**
 * @brief Issue a CPU pause hint.
 *
 * Useful inside spinlocks to reduce power usage and improve HT fairness.
 */
void atomic_cpu_pause(void);
