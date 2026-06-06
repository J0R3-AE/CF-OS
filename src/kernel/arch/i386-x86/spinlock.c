#include <arch/spinlock.h>
#include <arch/atomic.h>

void spinlock_init(spinlock_t *lock) 
{ atomic_store(&lock->lock, SPINLOCK_UNLOCKED);}

void spinlock_unlock(spinlock_t *lock) 
{ atomic_store(&lock->lock, SPINLOCK_UNLOCKED);}

u32 spinlock_state(spinlock_t *lock) 
{ return atomic_load(&lock->lock);}

void spinlock_force_unlock(spinlock_t *lock) 
{ lock->lock = SPINLOCK_UNLOCKED;}

void spinlock_force_lock(spinlock_t *lock) 
{ lock->lock = SPINLOCK_LOCKED;}

void spinlock_wait(spinlock_t *lock)
{while (atomic_load(&lock->lock) != SPINLOCK_UNLOCKED){atomic_cpu_pause();}}

void spinlock_lock(spinlock_t *lock)
{while (1)
    {u32 prev = atomic_cmpxchg(&lock->lock, SPINLOCK_UNLOCKED, SPINLOCK_LOCKED);
        if (prev == SPINLOCK_UNLOCKED){return;}
        while (atomic_load(&lock->lock) == SPINLOCK_LOCKED){atomic_cpu_pause();}}
}

u32 spinlock_trylock(spinlock_t *lock)
{u32 prev = atomic_cmpxchg(&lock->lock, SPINLOCK_UNLOCKED, SPINLOCK_LOCKED);
    return (prev == SPINLOCK_UNLOCKED) ? 1 : 0;}

u32 spinlock_destroy(spinlock_t *lock)
{u32 prev = atomic_cmpxchg(&lock->lock, SPINLOCK_UNLOCKED, SPINLOCK_DESTROYED);
    return (prev == SPINLOCK_UNLOCKED) ? 1 : 0;
}
