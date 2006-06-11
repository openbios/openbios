/*
 * arch/i386/libgcc/__divti3.c
 */

#include "asm/types.h"
#define NULL ((void *)0)

extern __uint128_t __udivmodti4(__uint128_t num, __uint128_t den, __uint128_t *rem);

__int128_t __divti3(__int128_t num, __int128_t den)
{
  int minus = 0;
  __int128_t v;

  if ( num < 0 ) {
    num = -num;
    minus = 1;
  }
  if ( den < 0 ) {
    den = -den;
    minus ^= 1;
  }

  v = __udivmodti4(num, den, NULL);
  if ( minus )
    v = -v;

  return v;
}
