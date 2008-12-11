#include "psr.h"
#include "asm/asi.h"
#define ASI_BP ASI_M_BYPASS
#define REGWIN_SZ   0x40

	.globl	__switch_context, __switch_context_nosave, __exit_context, halt

	.text
	.align	4

/*
 * Switch execution context
 * This saves registers in the stack, then
 * switches the stack, and restores everything from the new stack.
 * This function takes no argument. New stack pointer is
 * taken from global variable __context, and old stack pointer
 * is also saved to __context. This way we can just jump to
 * this routine to get back to the original context.
 */

/* XXX: totally bogus for sparc, need to save and restore all windows */
__switch_context:
	/* Save everything in current stack */
	set	__context, %g1
        st      %g2, [%g1 + 8]
        st      %g3, [%g1 + 12]
        st      %g4, [%g1 + 16]
        st      %g5, [%g1 + 20]
        st      %g6, [%g1 + 24]
        st      %g7, [%g1 + 28]

        st      %o0, [%g1 + 32]
        st      %o1, [%g1 + 36]
        st      %o2, [%g1 + 40]
        st      %o3, [%g1 + 44]
        st      %o4, [%g1 + 48]
        st      %o5, [%g1 + 52]
        st      %o6, [%g1 + 56]
        st      %o7, [%g1 + 60]

        st      %l0, [%g1 + 64]
        st      %l1, [%g1 + 68]
        st      %l2, [%g1 + 72]
        st      %l3, [%g1 + 76]
        st      %l4, [%g1 + 80]
        st      %l5, [%g1 + 84]
        st      %l6, [%g1 + 88]
        st      %l7, [%g1 + 92]

        st      %i0, [%g1 + 96]
        st      %i1, [%g1 + 100]
        st      %i2, [%g1 + 104]
        st      %i3, [%g1 + 108]
        st      %i4, [%g1 + 112]
        st      %i5, [%g1 + 116]
        st      %i6, [%g1 + 120]
        st      %i7, [%g1 + 124]

__switch_context_nosave:
	/* Interrupts are not allowed... */

	/* Turn on Supervisor, EnableFloating, and all the PIL bits.
	 * Also puts us in register window zero with traps off.
	 */
#if 0
	set	(PSR_PS | PSR_S | PSR_PIL | PSR_EF), %g2
	wr	%g2, 0x0, %psr
#endif

	/* Load all registers
	 */
	set	__context, %g1
        ld      [%g1], %g1
        ld      [%g1 + 8], %g2
        ld      [%g1 + 12], %g3
        ld      [%g1 + 16], %g4
        ld      [%g1 + 20], %g5
        ld      [%g1 + 24], %g6
        ld      [%g1 + 28], %g7

        ld      [%g1 + 32], %o0
        ld      [%g1 + 36], %o1
        ld      [%g1 + 40], %o2
        ld      [%g1 + 44], %o3
        ld      [%g1 + 48], %o4
        ld      [%g1 + 52], %o5
        ld      [%g1 + 56], %o6
        ld      [%g1 + 60], %o7

        ld      [%g1 + 64], %l0
        ld      [%g1 + 68], %l1
        ld      [%g1 + 72], %l2
        ld      [%g1 + 76], %l3
        ld      [%g1 + 80], %l4
        ld      [%g1 + 84], %l5
        ld      [%g1 + 88], %l6
        ld      [%g1 + 92], %l7

        ld      [%g1 + 96], %i0
        ld      [%g1 + 100], %i1
        ld      [%g1 + 104], %i2
        ld      [%g1 + 108], %i3
        ld      [%g1 + 112], %i4
        ld      [%g1 + 116], %i5
        ld      [%g1 + 120], %i6
        ld      [%g1 + 124], %i7

        ld      [%g1 + 128], %g1
       	/* Finally, load new %pc */
        jmp     %g1
         clr    %g1

__exit_context:
	/* Get back to the original context */
	call	__switch_context
	 nop

	/* We get here if the other context attempt to switch to this
	 * dead context. This should not happen. */

halt:
	b	halt
	 nop
