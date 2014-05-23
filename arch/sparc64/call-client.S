	.globl	sparc64_of_client_interface

	.data
	.align	8

	.skip	16384
openbios_stack:

client_stack:
	.xword	0

	.text
	.align	4
        .register %g2, #scratch
        .register %g3, #scratch
        .register %g6, #scratch
        .register %g7, #scratch
/*
	make some more space on stack since linux kernel only provides 128 bytes
	without memory to spill registers (used by gcc in -O0 mode)
*/

sparc64_of_client_interface:
	/* make sure caller's windows are on caller's stack */
	flushw

	/* Save globals on callers stack */
	add	%sp, -56, %sp

	stx	%g1, [%sp + 2047 + 0]
	stx	%g2, [%sp + 2047 + 8]
	stx	%g3, [%sp + 2047 + 16]
	stx	%g4, [%sp + 2047 + 24]
	stx	%g5, [%sp + 2047 + 32]
	stx	%g6, [%sp + 2047 + 40]
	stx	%g7, [%sp + 2047 + 48]

	/* Save existing stack and move to openbios stack */
	setx	client_stack, %g6, %g7
	stx	%sp, [%g7]
	setx	openbios_stack - 2047 - 192, %g6, %g7
	mov	%g7, %sp

	/* Call client inteface */
	save	%sp, -192, %sp

	call of_client_interface
	 mov %i0, %o0

	mov	%o0, %i0
	restore	%sp, 192, %sp

	/* Restore stack */
	setx	client_stack, %g6, %g7
	ldx	[%g7], %sp

	/* Restore globals */
	ldx	[%sp + 2047 + 0], %g1
	ldx	[%sp + 2047 + 8], %g2
	ldx	[%sp + 2047 + 16], %g3
	ldx	[%sp + 2047 + 24], %g4
	ldx	[%sp + 2047 + 32], %g5
	ldx	[%sp + 2047 + 40], %g6
	ldx	[%sp + 2047 + 48], %g7

	add	%sp, 56, %sp

	jmp	%o7+8
	 nop
