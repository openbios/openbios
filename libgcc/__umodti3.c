/*
 * arch/i386/libgcc/__umoddi3.c
 */

#include "asm/types.h"

extern __uint128_t __udivmodti4(__uint128_t num, __uint128_t den, __uint128_t *rem);

__uint128_t __umodti3(__uint128_t num, __uint128_t den)
{
  __uint128_t v;

  (void) __udivmodti4(num, den, &v);
  return v;
}
