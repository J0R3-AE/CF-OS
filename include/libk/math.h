#ifndef LIBK_MATH_H
#define LIBK_MATH_H

#include "libk/types.h"

/* basic */
int  abs_i32(int x);
int  min_i32(int a, int b);
int  max_i32(int a, int b);
int  clamp_i32(int v, int lo, int hi);

/* unsigned basic */
u32  min_u32(u32 a, u32 b);
u32  max_u32(u32 a, u32 b);
u32  clamp_u32(u32 v, u32 lo, u32 hi);

/* alignment */
u32  align_up(u32 value, u32 align);
u32  align_down(u32 value, u32 align);
u32  round_up_u32(u32 value, u32 multiple);
u32  round_down_u32(u32 value, u32 multiple);

/* division helpers */
u32  ceil_div_u32(u32 a, u32 b);
u32  floor_div_u32(u32 a, u32 b);

/* powers / roots */
u32  pow_u32(u32 base, u32 exp);
u32  mod_pow_u32(u32 base, u32 exp, u32 mod);
u32  sqrt_u32(u32 n);

/* gcd / lcm */
u32  gcd_u32(u32 a, u32 b);
u32  lcm_u32(u32 a, u32 b);

/* primes / factoring */
int  is_prime_u32(u32 n);
u32  next_prime_u32(u32 n);
u32  factor_smallest_u32(u32 n);

/* logs / bits */
u32  log2_floor_u32(u32 n);
u32  log2_ceil_u32(u32 n);
u32  popcount_u32(u32 n);
int  parity_u32(u32 n);

/* sequences */
u32  factorial_u32(u32 n);
u32  fibonacci_u32(u32 n);

#endif /* LIBK_MATH_H */