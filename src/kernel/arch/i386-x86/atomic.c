#include "arch/atomic.h"
#include "libk/log.h"

u32 atomic_exchange(volatile u32 *ptr, u32 val)
{
    __asm__ volatile("lock xchg %0, %1" : "+r"(val), "+m"(*ptr)::"memory");
    return val;
}

u32 atomic_cmpxchg(volatile u32 *ptr, u32 expected, u32 desired)
{
    {
        u32 prev;
        __asm__ volatile("lock cmpxchg %2, %1" : "=a"(prev), "+m"(*ptr) : "r"(desired), "0"(expected) : "memory");
        return prev;
    }
}

u32 atomic_fetch_add(volatile u32 *ptr, u32 inc)
{
    u32 old, new;
    do
    {
        old = __atomic_load_n(ptr, __ATOMIC_RELAXED);
        new = old + inc;
    } while (atomic_cmpxchg(ptr, old, new) != old);
    return old;
}

u32 atomic_fetch_sub(volatile u32 *ptr, u32 dec)
{
    u32 old, new;
    do
    {
        old = __atomic_load_n(ptr, __ATOMIC_RELAXED);
        new = old - dec;
    } while (atomic_cmpxchg(ptr, old, new) != old);
    return old;
}

void atomic_add(volatile u32 *ptr, u32 inc)
{
    (void)atomic_fetch_add(ptr, inc);
}
void atomic_sub(volatile u32 *ptr, u32 dec)
{
    (void)atomic_fetch_sub(ptr, dec);
}
u32 atomic_load(volatile u32 *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
}
void atomic_store(volatile u32 *ptr, u32 val)
{
    __atomic_store_n(ptr, val, __ATOMIC_RELEASE);
}
void atomic_cpu_pause(void)
{
    __asm__ volatile("pause");
}