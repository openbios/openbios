#include "autoconf.h"
#include "asm/asmdefs.h"
#include "asm/processor.h"

#ifdef CONFIG_PPC_64BITSUPPORT
	#define STACKFRAME_MINSIZE 48
#else /* !CONFIG_PPC_64BITSUPPORT */
	#define STACKFRAME_MINSIZE 16
#endif

	/* According to IEEE 1275, PPC bindings:
	 *
	 * 	MSR = FP, ME + (DR|IR)
	 *	r1 = stack (32 K + 32 bytes link area above)
	 *	r5 = client interface handler
	 *	r6 = address of client program arguments (unused)
	 *	r7 = length of client program arguments (unused)
         *
         *      Yaboot and Linux use r3 and r4 for initrd address and size
	 */

	/* void call_elf( arg1, arg2, entry ) */
_GLOBAL(call_elf):
	mflr	r0
	PPC_STLU r1, -STACKFRAME_MINSIZE(r1)
	PPC_STL  r0, (STACKFRAME_MINSIZE + PPC_LR_STKOFF)(r1)
	mtlr	r5
	LOAD_REG_IMMEDIATE(r8, saved_stack)		// save our stack pointer
	PPC_STL r1,0(r8)
	mfsdr1	r1
	addi	r1, r1, -32768		/* - 32 KiB exception stack */
	addis	r1, r1, -1			/* - 64 KiB stack */
	LOAD_REG_IMMEDIATE(r5, of_client_callback)	// r5 = callback
	li	r6,0			// r6 = address of client program arguments (unused)
	li	r7,0			// r7 = length of client program arguments (unused)
	li	r0,MSR_FP | MSR_ME | MSR_DR | MSR_IR
	MTMSRD(r0)
	blrl

#ifdef CONFIG_PPC64
    /* Restore SF bit */
    LOAD_REG_IMMEDIATE(r0, MSR_SF | MSR_FP | MSR_ME | MSR_DR | MSR_IR)
    MTMSRD(r0)
#endif
	LOAD_REG_IMMEDIATE(r8, saved_stack)		// restore stack pointer
	mr	r1,r8
	PPC_LL r0, (STACKFRAME_MINSIZE + PPC_LR_STKOFF)(r1)
	mtlr	r0
	addi	r1, r1, STACKFRAME_MINSIZE
	// XXX: should restore r12-r31 etc..
	// we should not really come here though
	blrl
