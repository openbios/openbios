/*   -*- asm -*-
 *
 *   Creation Date: <2001/02/03 19:38:07 samuel>
 *   Time-stamp: <2003/07/08 18:55:50 samuel>
 *
 *	<asmdefs.h>
 *
 *	Common assembly definitions
 *
 *   Copyright (C) 2001, 2002, 2003 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#ifndef _H_ASMDEFS
#define _H_ASMDEFS

/************************************************************************/
/*	High/low halfword compatibility macros				*/
/************************************************************************/

#ifndef __darwin__
#define 	ha16( v )	(v)##@ha
#define 	hi16( v )	(v)##@h
#define 	lo16( v )	(v)##@l
#endif
#define		HA(v)		ha16(v)
#define		HI(v)		hi16(v)
#define		LO(v)		lo16(v)

/* from Linux: include/asm-powerpc/ppc_asm.h */
/*
 * Copyright (C) 1995-1999 Gary Thomas, Paul Mackerras, Cort Dougan.
 */

/* General Purpose Registers (GPRs) */

#define	r0	0
#define	r1	1
#define	r2	2
#define	r3	3
#define	r4	4
#define	r5	5
#define	r6	6
#define	r7	7
#define	r8	8
#define	r9	9
#define	r10	10
#define	r11	11
#define	r12	12
#define	r13	13
#define	r14	14
#define	r15	15
#define	r16	16
#define	r17	17
#define	r18	18
#define	r19	19
#define	r20	20
#define	r21	21
#define	r22	22
#define	r23	23
#define	r24	24
#define	r25	25
#define	r26	26
#define	r27	27
#define	r28	28
#define	r29	29
#define	r30	30
#define	r31	31

/************************************************************************/
/*	MISC								*/
/************************************************************************/

#ifdef __powerpc64__
#define LOAD_REG_IMMEDIATE(D, x) \
	lis  (D),      (x)@highest ; \
	ori  (D), (D), (x)@higher ; \
	sldi (D), (D), 32 ; \
	oris (D), (D), (x)@h ; \
	ori  (D), (D), (x)@l
#else
#define LOAD_REG_IMMEDIATE(D, x) \
	lis  (D),      HA(x) ; \
	addi (D), (D), LO(x)
#endif

#ifndef __darwin__
#define GLOBL( name )		.globl name ; name
#define EXTERN( name )		name
#else
/* an underscore is needed on Darwin */
#define GLOBL( name )		.globl _##name ; name: ; _##name
#define EXTERN( name )		_##name
#endif

#define	BIT(n)		(1<<(31-(n)))

#endif   /* _H_ASMDEFS */
