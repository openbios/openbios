/*
 * arch/i386/libgcc/__divdi3.c
 */

#include "asm/types.h"
#define NULL ((void *)0)

extern __uint128_t __udivmodti4(__uint128_t num, __uint128_t den, __uint128_t *rem);

__uint128_t __udivti3(__uint128_t num, __uint128_t den)
{
  return __udivmodti4(num, den, NULL);
}
