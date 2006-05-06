/**
 ** Standalone startup code for Linux PROM emulator.
 ** Copyright 1999 Pete A. Zaitcev
 ** This code is licensed under GNU General Public License.
 **/
/*
 * $Id: head.S,v 1.12 2002/07/23 05:47:09 zaitcev Exp $
 */

#include "psr.h"
#include "asi.h"
#include "asm/crs.h"

#define PHYS_JJ_EEPROM	0x71200000

#define WRITE_PAUSE    nop; nop; nop; /* Have to do this after %wim/%psr chg */

        .globl	entry, _entry

	.section ".text", "ax"
	.align	8

/*
 * Entry point
 * We start execution from here.
 */
_entry:
entry:
        /* Switch to our main context.
         * Main context is statically defined in C.
         */
        /*
        call	__switch_context_nosave
         nop
        */
        /* XXX no switching yet */

        set	PHYS_JJ_EEPROM + 0x30, %g1
        lda	[%g1] ASI_M_BYPASS, %g1
        ! map PROLDATA to PROLBASE+PROLSIZE to end of ram
        !set	PROLSIZE+0x1000-PROLDATA+PROLBASE, %g2	! add 0x1000 for temp tables
        set     _end, %g3
        set     0xfff, %g2
        add     %g3, %g2, %g3
        andn    %g3, %g2, %g3
        set     _data, %g2
        sub     %g3, %g2, %g3
        set     0x1000, %g4                     ! add 0x1000 for page tables
        add     %g4, %g3, %g2
        sub	%g1, %g2, %g2			! start of private memory
        srl	%g2, 0x4, %g7			! ctx table at s+0x0
        add	%g2, 0x400, %g3			! l1 table at s+0x400
        srl	%g3, 0x4, %g3
        or	%g3, 0x1, %g3
        sta	%g3, [%g2] ASI_M_BYPASS
        add	%g2, 0x400, %g2			! s+0x400
        add	%g2, 0x400, %g3			! l2 table for ram (00xxxxxx) at s+0x800
        srl	%g3, 0x4, %g3
        or	%g3, 0x1, %g3
        sta	%g3, [%g2] ASI_M_BYPASS
        add	%g2, 0x500, %g3			! l2 table for rom (ffxxxxxx) at s+0x900
        add	%g2, 0x3fc, %g2			! s+0x7fc
        srl	%g3, 0x4, %g3
        or	%g3, 0x1, %g3
        sta	%g3, [%g2] ASI_M_BYPASS
        add	%g2, 0x4, %g2			! s+0x800
        set     0x40, %g6
        set	((7 << 2) | 2), %g3		! 7 = U: --- S: RWX (main memory)
1:      sta	%g3, [%g2] ASI_M_BYPASS
        add     %g2, 4, %g2
        deccc   %g6
        bne     1b
         nop
                                                ! s+0x900
        add	%g2, 0xa00 - 0x900, %g3		! l3 table for rom at s+0xa00
        add	%g2, 0x0d0, %g2			! s+0x9d0
        srl	%g3, 0x4, %g3
        or	%g3, 0x1, %g3
        sta	%g3, [%g2] ASI_M_BYPASS
        add	%g2, 4, %g2			! s+0x9d4
        add	%g2, 0xb00 - 0x9d4, %g3		! 2nd l3 table for rom at s+0xb00
        srl	%g3, 0x4, %g3
        or	%g3, 0x1, %g3
        sta	%g3, [%g2] ASI_M_BYPASS
        add	%g2, 4, %g2			! s+0x9d8
        add	%g2, 0xc00 - 0x9d8, %g3		! 3rd l3 table for rom at s+0xc00
        srl	%g3, 0x4, %g3
        or	%g3, 0x1, %g3
        sta	%g3, [%g2] ASI_M_BYPASS
        add	%g2, 4, %g2			! s+0x9dc
        add	%g2, 0xd00 - 0x9dc, %g3		! 4th l3 table for rom at s+0xd00
        srl	%g3, 0x4, %g3
        or	%g3, 0x1, %g3
        sta	%g3, [%g2] ASI_M_BYPASS
        add	%g2, 4, %g2			! s+0x9e0
        add	%g2, 0xd00 - 0x9e0, %g3		! 5th l3 table for rom at s+0xe00
        srl	%g3, 0x4, %g3
        or	%g3, 0x1, %g3
        sta	%g3, [%g2] ASI_M_BYPASS
        add	%g2, 0xa00-0x9e0, %g2			! s+0xa00

        !set	(PROLDATA-PROLBASE)/0x1000, %g6 ! # of .text pages
        set	_start, %g3
        set	_data, %g6
        set	0x1000, %g5
        sub     %g6, %g3, %g6
        srl     %g6, 12, %g6                    ! # of .text pages
1:      srl	%g3, 0x4, %g4
        or	%g4, ((7 << 2) | 2), %g4	! 4 = U: --X S: --X (rom, execute only)
        sta	%g4, [%g2] ASI_M_BYPASS
        add	%g2, 4, %g2
        add	%g3, %g5, %g3
        deccc	%g6
        bne	1b
         nop
#if 0
        !set	(PROLDATA-PROLRODATA)/0x1000, %g6 ! # of .rodata pages
        set	_data, %g6
        set	_rodata, %g4
        sub     %g6, %g4, %g6
        srl     %g6, 12, %g6                             ! # of .bss pages
1:      srl	%g3, 0x4, %g4
        or	%g4, ((0 << 2) | 2), %g4	! 0 = U: R-- S: R-- (rom, read only)
        sta	%g4, [%g2] ASI_M_BYPASS
        add	%g2, 4, %g2
        add	%g3, %g5, %g3
        deccc	%g6
        bne	1b
         nop
#endif
        !set	(PROLBASE+PROLSIZE-PROLDATA)/0x1000, %g6 ! # of .bss pages
        set	_end, %g6
        set	_data, %g4
        sub     %g6, %g4, %g6
        srl     %g6, 12, %g6                             ! # of .bss pages
        set	0x1000, %g4
        sll	%g7, 0x4, %g3
        add	%g4, %g3, %g3
1:      srl	%g3, 0x4, %g4
        or	%g4, ((7 << 2) | 2), %g4        ! 5 = U: R-- S: RW- (data area, read/write)
        sta	%g4, [%g2] ASI_M_BYPASS
        add	%g2, 4, %g2
        add	%g3, %g5, %g3
        deccc	%g6
        bne	1b
         nop

        mov	%g1, %g3

        set     AC_M_CTPR, %g2
        sta     %g7, [%g2] ASI_M_MMUREGS	! set ctx table ptr
        set	1, %g1
        sta     %g1, [%g0] ASI_M_MMUREGS	! enable mmu

        /*
         * The code which enables traps is a simplified version of
         * kernel head.S.
         *
         * We know number of windows as 8 so we do not calculate them.
         * The deadwood is here for any case.
         */

        /* Turn on Supervisor, EnableFloating, and all the PIL bits.
         * Also puts us in register window zero with traps off.
         */
        set	(PSR_PS | PSR_S | PSR_PIL | PSR_EF), %g2
        wr	%g2, 0x0, %psr
        WRITE_PAUSE

        /* Copy the DATA section from ROM. */
        set	_data - 4, %o0           ! First address of DATA
        set	_bss, %o1                ! Last address of DATA
        ba	2f
         nop
1:
        lda      [%o0] ASI_M_BYPASS, %g1
        st	%g1, [%o0]
2:
        subcc	%o0, %o1, %g0
        bl	1b
         add	%o0, 0x4, %o0

        /* Zero out our BSS section. */
        set	_bss - 4, %o0           ! First address of BSS
        set	_end, %o1               ! Last address of BSS
        ba	2f
         nop
1:
        st	%g0, [%o0]
2:
        subcc	%o0, %o1, %g0
        bl	1b
         add	%o0, 0x4, %o0

        set     _estack, %sp
        mov     0, %fp
        mov     2, %g1
        wr      %g1, 0x0, %wim			! make window 1 invalid
        WRITE_PAUSE

        set     trap_table, %g1
        wr      %g1, 0x0, %tbr

        set     qemu_mem_size, %g1
        st      %g3, [%g1]
        
        sll     %g7, 4, %g7                     ! Store va->pa conversion factor
        set     _data - 0x1000, %g1
        sub     %g1, %g7, %g7
        set     va_shift, %g1
        st      %g7, [%g1]
        
        /* Finally, turn on traps so that we can call c-code. */
        rd	%psr, %g3
        wr	%g3, 0x0, %psr
        WRITE_PAUSE

        wr	%g3, PSR_ET, %psr
        WRITE_PAUSE

        call    openbios
         nop

        /* We get here when the main context switches back to
         * the boot context.
         * Return to previous bootloader.
         */
        ret
         nop

