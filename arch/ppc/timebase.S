#include "asm/asmdefs.h"
#include "asm/processor.h"

/*
 * unsigned long long _get_ticks(void);
 */
GLOBL(_get_ticks):
1:      mftbu   r3
        mftb    r4
        mftbu   r5
        cmpw    0,r3,r5
        bne     1b
        blr

/*
 * Delay for a number of ticks
 */
GLOBL(_wait_ticks):
        mflr    r8              /* save link register */
        mr      r7, r3          /* save tick count */
        bl      _get_ticks       /* Get start time */

        /* Calculate end time */
        addc    r7, r4, r7      /* Compute end time lower */
        addze   r6, r3          /*     and end time upper */

1:      bl      _get_ticks       /* Get current time */
        subfc   r4, r4, r7      /* Subtract current time from end time */
        subfe.  r3, r3, r6
        bge     1b              /* Loop until time expired */

        mtlr    r8              /* restore link register */
        blr
