#include "psr.h"
#include "asi.h"
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

__switch_context_nosave:
	/* Interrupts are not allowed... */

	/* Turn on Supervisor, EnableFloating, and all the PIL bits.
	 * Also puts us in register window zero with traps off.
	 */
	set	(PSR_PS | PSR_S | PSR_PIL | PSR_EF), %g2
	wr	%g2, 0x0, %psr
	
	/* Load all registers
	 */
	set	__context, %g1
        lda     [%g1] ASI_BP, %g1
        add     %g1, 8, %g1     /* skip %g0 and %g1*/

#define load_reg(reg) lda [%g1] ASI_BP, reg ; add %g1, 4, %g1

        load_reg(%g2)
        load_reg(%g3)
        load_reg(%g4)
        load_reg(%g5)
        load_reg(%g6)
        load_reg(%g7)

        load_reg(%o0)
        load_reg(%o1)
        load_reg(%o2)
        load_reg(%o3)
        load_reg(%o4)
        load_reg(%o5)
        load_reg(%o6)
        load_reg(%o7)

        load_reg(%l0)
        load_reg(%l1)
        load_reg(%l2)
        load_reg(%l3)
        load_reg(%l4)
        load_reg(%l5)
        load_reg(%l6)
        load_reg(%l7)

        load_reg(%i0)
        load_reg(%i1)
        load_reg(%i2)
        load_reg(%i3)
        load_reg(%i4)
        load_reg(%i5)
        load_reg(%i6)
        load_reg(%i7)

       	/* Finally, load new %pc */
        lda     [%g1] ASI_BP, %g1
        jmp     %g1
	 nop

__exit_context:
	/* Get back to the original context */
	call	__switch_context
	 nop

	/* We get here if the other context attempt to switch to this
	 * dead context. This should not happen. */

halt:
	b	halt
	 nop
