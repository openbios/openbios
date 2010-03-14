/* tag: the main file which includes all the prim code headers
 *
 * Copyright (C) 2003 Patrick Mauritz, Stefan Reinauer
 *
 * see the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "openbios/config.h"
#include "openbios/sysinclude.h"
#include "kernel/stack.h"
#include "kernel/kernel.h"
#include "dict.h"

/*
 * cross platform abstraction
 */

#include "cross.h"

/*
 * Code Field Address (CFA) definitions (DOCOL and the like)
 */

#include "internal.c"

/* include forth primitives needed to set up
 * all the words described in IEEE1275-1994.
 */

#include "forth.c"

/* words[] is a function array of all native code functions in used by
 * the dictionary, i.e. CFAs and primitives.
 * Any change here needs a matching change in the primitive word's
 * name list that is kept for bootstrapping in arch/unix/unix.c
 *
 * NOTE: THIS LIST SHALL NOT CHANGE (EXCEPT MANDATORY ADDITIONS AT
 * THE END). ANY OTHER CHANGE WILL BREAK COMPATIBILITY TO OLDER
 * BINARY DICTIONARIES.
 */
static forth_word * const words[] = {
	/*
	 * CFAs and special words
	 */
	semis,
	docol,
	lit,
	docon,
	dovar,
	dodefer,
	dodoes,
	dodo,
	doisdo,
	doloop,
	doplusloop,
	doival,
	doivar,
	doidefer,

	/*
	 * primitives
	 */
	fdup,			/* dup     */
	twodup,			/* 2dup    */
	isdup,			/* ?dup    */
	over,			/* over    */
	twoover,		/* 2over   */
	pick,			/* pick    */
	drop,			/* drop    */
	twodrop,		/* 2drop   */
	nip,			/* nip     */
	roll,			/* roll    */
	rot,			/* rot     */
	minusrot,		/* -rot    */
	swap,			/* swap    */
	twoswap,		/* 2swap   */
	tor,			/* >r      */
	rto,			/* r>      */
	rfetch,			/* r@      */
	depth,			/* depth   */
	depthwrite,		/* depth!  */
	rdepth,			/* rdepth  */
	rdepthwrite,		/* rdepth! */
	plus,			/* +       */
	minus,			/* -       */
	mult,			/* *       */
	umult,			/* u*      */
	mudivmod,		/* mu/mod  */
	forthabs,		/* abs     */
	negate,			/* negate  */
	max,			/* max     */
	min,			/* min     */
	lshift,			/* lshift  */
	rshift,			/* rshift  */
	rshifta,		/* >>a     */
	and,			/* and     */
	or,			/* or      */
	xor,			/* xor     */
	invert,			/* invert  */
	dplus,			/* d+      */
	dminus,			/* d-      */
	mmult,			/* m*      */
	ummult,			/* um*     */
	fetch,			/* @       */
	cfetch,			/* c@      */
	wfetch,			/* w@      */
	lfetch,			/* l@      */
	store,			/* !       */
	plusstore,		/* +!      */
	cstore,			/* c!      */
	wstore,			/* w!      */
	lstore,			/* l!      */
	equals,			/* =       */
	greater,		/* >       */
	less,			/* <       */
	ugreater,		/* u>      */
	uless,			/* u<      */
	spfetch,		/* sp@     */
	fmove,			/* move    */
	ffill,			/* fill    */
	emit,			/* emit    */
	iskey,			/* key?    */
	key,			/* key     */
	execute,		/* execute */
	here,			/* here    */
	herewrite,		/* here!   */
	dobranch,		/* dobranch     */
	docbranch,		/* do?branch    */
	unalignedwordread,	/* unaligned-w@ */
	unalignedwordwrite,	/* unaligned-w! */
	unalignedlongread,	/* unaligned-l@ */
	unalignedlongwrite,	/* unaligned-l! */
	iocfetch,		/* ioc@    */
	iowfetch,		/* iow@    */
	iolfetch,		/* iol@    */
	iocstore,		/* ioc!    */
	iowstore,		/* iow!    */
	iolstore,		/* iol!    */
	loop_i,			/* i       */
	loop_j,			/* j       */
	call,			/* call    */
	sysdebug,		/* sys-debug */
	do_include,		/* $include */
	do_encode_file,		/* $encode-file */
	do_debug_xt,		/* (debug  */
	do_debug_off,		/* (debug-off) */
};
