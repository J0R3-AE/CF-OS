/**
 * @file spinlock.h
 * @brief Simple x86 spinlock implementation for kernel synchronization.
 *
 * This header defines a minimal spinlock type and associated operations
 * for mutual exclusion in low‑level kernel code. The lock uses a single
 * 32‑bit atomic field where:
 *   - 0 = unlocked
 *   - 1 = locked
 *
 * Spinlocks are intended for short, non‑blocking critical sections and
 * must not be used in contexts where sleeping is required.
 */

#pragma once

#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* Spinlock State                                                             */
/* -------------------------------------------------------------------------- */

/**
 * @enum spinlock_state
 * @brief Possible states of a spinlock.
 */
enum spinlock_state
{
    SPINLOCK_DESTROYED = -1, /**< Lock has been destroyed and is unusable. */
    SPINLOCK_UNLOCKED = 0,   /**< Lock is free. */
    SPINLOCK_LOCKED = 1,     /**< Lock is held. */
    SPINLOCK_FAILED = 2      /**< Operation failed (e.g., invalid lock). */
};

/* -------------------------------------------------------------------------- */
/* Spinlock Structure                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @struct spinlock
 * @brief Basic spinlock containing a single atomic lock word.
 *
 * The `lock` field is modified using atomic operations to ensure
 * correct behavior on multiprocessor systems.
 */
typedef struct spinlock
{
    volatile u32 lock; /**< 0 = unlocked, 1 = locked. */
} spinlock_t;

/* -------------------------------------------------------------------------- */
/* Spinlock API                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize a spinlock to the unlocked state.
 *
 * @param lock Pointer to the spinlock.
 */
void spinlock_init(spinlock_t *lock);

/**
 * @brief Acquire the spinlock, blocking (spinning) until available.
 *
 * Uses an atomic exchange or compare‑and‑swap operation. The caller
 * will spin until the lock transitions to the unlocked state.
 *
 * @param lock Pointer to the spinlock.
 */
void spinlock_lock(spinlock_t *lock);

/**
 * @brief Release the spinlock.
 *
 * Sets the lock state back to unlocked. Only the holder should call this.
 *
 * @param lock Pointer to the spinlock.
 */
void spinlock_unlock(spinlock_t *lock);

/**
 * @brief Destroy a spinlock.
 *
 * Marks the lock as destroyed. Useful for debugging or lifecycle tracking.
 *
 * @param lock Pointer to the spinlock.
 * @return New state (SPINLOCK_DESTROYED).
 */
u32 spinlock_destroy(spinlock_t *lock);

/**
 * @brief Attempt to acquire the lock without blocking.
 *
 * @param lock Pointer to the spinlock.
 * @return 1 if acquired, 0 if already locked.
 */
u32 spinlock_trylock(spinlock_t *lock);

/**
 * @brief Get the current state of the spinlock.
 *
 * @param lock Pointer to the spinlock.
 * @return Current lock state (0 or 1).
 */
u32 spinlock_state(spinlock_t *lock);

/**
 * @brief Wait until the lock becomes unlocked (does not acquire it).
 *
 * Useful for synchronization patterns where the caller must wait for
 * a critical section to complete without entering it.
 *
 * @param lock Pointer to the spinlock.
 */
void spinlock_wait(spinlock_t *lock);

/**
 * @brief Forcefully unlock the spinlock.
 *
 * Sets the lock to unlocked regardless of its previous state.
 * Should only be used in exceptional situations.
 *
 * @param lock Pointer to the spinlock.
 */
void spinlock_force_unlock(spinlock_t *lock);

/**
 * @brief Forcefully lock the spinlock.
 *
 * Sets the lock to locked regardless of its previous state.
 * Should be used with caution.
 *
 * @param lock Pointer to the spinlock.
 */
void spinlock_force_lock(spinlock_t *lock);
