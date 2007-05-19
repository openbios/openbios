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

#define PHYS_JJ_EEPROM	0x71200000  /* [2000] MK48T08 */
#define PHYS_JJ_INTR0	0x71E00000  /* CPU0 interrupt control registers */

#define PHYS_SS10_EEPROM	0xf1200000
#define PHYS_SS10_INTR0 	0xf1400000

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

        ! Check if this is QEMU for SS-5
        set PHYS_JJ_EEPROM, %g1
        lduba   [%g1] ASI_M_BYPASS, %g2
        cmp     %g2, 'Q'
        bne     ss10
         inc    %g1
        lduba   [%g1] ASI_M_BYPASS, %g2
        cmp     %g2, 'E'
        bne     ss10
         inc    %g1
        lduba   [%g1] ASI_M_BYPASS, %g2
        cmp     %g2, 'M'
        bne     ss10
         inc    %g1
        lduba   [%g1] ASI_M_BYPASS, %g2
        cmp     %g2, 'U'
        bne     ss10

        ! Ok, this is SS-5
        mov     0x80, %y
        ! Check if this not the first SMP CPU, if so, bypass PROM entirely
        set     PHYS_JJ_EEPROM + 0x2E, %g1
        lduba   [%g1] ASI_M_BYPASS, %g2
        set	PHYS_JJ_EEPROM + 0x30, %g1
        lda	[%g1] ASI_M_BYPASS, %g1
        tst     %g2
        bz      first_cpu
         nop

        set     PHYS_JJ_INTR0 + 0x04, %g1
        sll     %g2, 12, %g2
        add     %g1, %g2, %g2
        set     0xffffffff, %g1
        sta     %g1, [%g2] ASI_M_BYPASS         ! clear softints
        add     %g2, 4, %g2
        sta     %g0, [%g2] ASI_M_BYPASS         ! clear softints
        set     PHYS_JJ_EEPROM + 0x3C, %g1
        lda     [%g1] ASI_M_BYPASS, %g1
        set     AC_M_CTPR, %g2
        sta     %g1, [%g2] ASI_M_MMUREGS        ! set ctx table ptr
        set     PHYS_JJ_EEPROM + 0x40, %g1
        lda     [%g1] ASI_M_BYPASS, %g1
        set     AC_M_CXR, %g2
        sta     %g1, [%g2] ASI_M_MMUREGS        ! set context
        set     PHYS_JJ_EEPROM + 0x38, %g1
        lda     [%g1] ASI_M_BYPASS, %g2
        set     1, %g1
        jmp     %g2                             ! jump to kernel
         sta    %g1, [%g0] ASI_M_MMUREGS        ! enable mmu

bad_nvram:
        ! Unknown machine, freeze
        b       bad_nvram
         nop
ss10:
        set PHYS_SS10_EEPROM, %g1
        lduba   [%g1] ASI_M_CTL, %g2
        cmp     %g2, 'Q'
        bne     bad_nvram
         inc    %g1
        lduba   [%g1] ASI_M_CTL, %g2
        cmp     %g2, 'E'
        bne     bad_nvram
         inc    %g1
        lduba   [%g1] ASI_M_CTL, %g2
        cmp     %g2, 'M'
        bne     bad_nvram
         inc    %g1
        lduba   [%g1] ASI_M_CTL, %g2
        cmp     %g2, 'U'
        bne     bad_nvram
        
        ! Ok, this is SS-10
        mov     0x72, %y
        ! Check if this not the first SMP CPU, if so, bypass PROM entirely
        set     PHYS_SS10_EEPROM + 0x2E, %g1
        lduba   [%g1] ASI_M_CTL, %g2
        set	PHYS_SS10_EEPROM + 0x30, %g1
        lda	[%g1] ASI_M_CTL, %g1
        tst     %g2
        bz      first_cpu
         nop
        set     PHYS_SS10_INTR0 + 0x04, %g1
        sll     %g2, 12, %g2
        add     %g1, %g2, %g2
        set     0xffffffff, %g1
        sta     %g1, [%g2] ASI_M_CTL         ! clear softints
        add     %g2, 4, %g2
        sta     %g0, [%g2] ASI_M_CTL         ! clear softints
        set     PHYS_SS10_EEPROM + 0x3C, %g1
        lda     [%g1] ASI_M_CTL, %g1
        set     AC_M_CTPR, %g2
        sta     %g1, [%g2] ASI_M_MMUREGS        ! set ctx table ptr
        set     PHYS_JJ_EEPROM + 0x40, %g1
        lda     [%g1] ASI_M_CTL, %g1
        set     AC_M_CXR, %g2
        sta     %g1, [%g2] ASI_M_MMUREGS        ! set context
        set     PHYS_SS10_EEPROM + 0x38, %g1
        lda     [%g1] ASI_M_CTL, %g2
        set     1, %g1
        jmp     %g2                             ! jump to kernel
         sta    %g1, [%g0] ASI_M_MMUREGS        ! enable mmu

first_cpu:
        /* Create temporary page tables and map the ROM area to end of
	RAM. This will be done properly in iommu.c later. */
        set     _end, %g3
        set     0xfff, %g2
        add     %g3, %g2, %g3
        andn    %g3, %g2, %g3
        set     _start, %g2
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
#if 0
        set     0x40, %g6
        set	((7 << 2) | 2), %g3		! 7 = U: --- S: RWX (main memory)
1:      sta	%g3, [%g2] ASI_M_BYPASS
        add     %g2, 4, %g2
        deccc   %g6
        bne     1b
         nop
#else
        add     %g2, 0x100, %g2
#endif
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

        /* Use end of ram for code, rodata, data, and bss
	sections. SunOS wants to write to trap table... */
        set	_end, %g6
        set	_start, %g4
        sub     %g6, %g4, %g6
        srl     %g6, 12, %g6                    ! # of all pages
        set	0x1000, %g5
        sll	%g7, 0x4, %g3
        add	%g5, %g3, %g3                   ! ctx table + 0x1000
1:      srl	%g3, 0x4, %g4
        or	%g4, ((7 << 2) | 2), %g4        ! 7 = U: --- S: RWX
        sta	%g4, [%g2] ASI_M_BYPASS
        add	%g2, 4, %g2
        add	%g3, %g5, %g3
        deccc	%g6
        bne	1b
         nop

        mov	%g1, %g6                        ! %g6 = memory size

        /* Copy the code, rodata and data sections from ROM. */
        set	0x1000 - 4, %g4
        sll	%g7, 0x4, %g3
        add	%g4, %g3, %g3                   ! ctx table + 0x1000 - 4
        set	_start - 4, %g4                 ! First address of TEXT
        set	_bss, %g5                       ! Last address of DATA
        ba	2f
         nop
1:
        lda     [%g4] ASI_M_BYPASS, %g1
        sta	%g1, [%g3] ASI_M_BYPASS
2:
        cmp     %g4, %g5
        add     %g3, 0x4, %g3
        bl      1b
         add    %g4, 0x4, %g4


        set     AC_M_CTPR, %g2
        sta     %g7, [%g2] ASI_M_MMUREGS	! set ctx table ptr
        set     AC_M_CXR, %g2
        sta     %g0, [%g2] ASI_M_MMUREGS	! context 0
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

        /* Zero out our BSS section. */
        set	_bss - 4, %o0           ! First address of BSS
        set	_estack - 4, %o1        ! Last address of BSS
        ba	2f
         nop
1:
        st	%g0, [%o0]
2:
        subcc	%o0, %o1, %g0
        bl	1b
         add	%o0, 0x4, %o0

        mov     2, %g1
        wr      %g1, 0x0, %wim			! make window 1 invalid
        WRITE_PAUSE

        set     trap_table, %g1
        wr      %g1, 0x0, %tbr

        set     qemu_mem_size, %g1
        st      %g6, [%g1]
        
        sll     %g7, 4, %g7                     ! Store va->pa conversion factor
        set     _start - 0x1000, %g1
        sub     %g1, %g7, %g7
        set     va_shift, %g1
        st      %g7, [%g1]

        set     qemu_machine_type, %g1
        mov     %y, %g2
        st      %g2, [%g1]
        
        /* Finally, turn on traps so that we can call c-code. */
        rd	%psr, %g3
        wr	%g3, 0x0, %psr
        WRITE_PAUSE

        wr	%g3, PSR_ET, %psr
        WRITE_PAUSE

        call    __switch_context_nosave
         nop

        /* We get here when the main context switches back to
         * the boot context.
         * Return to previous bootloader.
         */
        ret
         nop

