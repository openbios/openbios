/*
 * <vectors.S>
 *
 * Sparc V9 Trap Table(s) with SpitFire/Cheetah extensions.
 *
 *   Copyright (C) 1996, 2001 David S. Miller (davem@caip.rutgers.edu)
 * 
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2 as published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *   MA  02110-1301, USA.
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License V2
 *   as published by the Free Software Foundation
 */

#define __ASSEMBLY__
#include "pstate.h"
#include "asi.h"
#include "ttable.h"
#define ASI_BP ASI_PHYS_BYPASS_EC_E
#define SER_ADDR 0x1fe020003f8

        .section ".text.vectors", "ax"
        .align 16384
/* Sparc64 trap table */
        .globl trap_table, __divide_error
        .register %g2, #scratch
        .register %g3, #scratch
        .register %g6, #scratch
        .register %g7, #scratch
trap_table:
#undef  TRAP_IRQ
#define TRAP_IRQ(routine, level)			\
		ba routine; nop; nop; nop; nop; nop; nop; nop;
#undef  BTRAP
#define BTRAP(lvl)					\
		 ba bug; mov lvl, %g1; nop; nop; nop; nop; nop; nop;
#undef  BTRAPTL1
#define BTRAPTL1(lvl) BTRAP(lvl)
#define BTRAPS(x) BTRAP(x) BTRAP(x+1) BTRAP(x+2) BTRAP(x+3) BTRAP(x+4) BTRAP(x+5) BTRAP(x+6) BTRAP(x+7)

#define STACK_BIAS		2047
	.globl	sparc64_ttable_tl0, sparc64_ttable_tl1
sparc64_ttable_tl0:
                ba entry; nop; nop; nop; nop; nop; nop; nop;
		BTRAP(0x01) BTRAP(0x02) BTRAP(0x03) BTRAP(0x04) BTRAP(0x05) BTRAP(0x06) BTRAP(0x07)
                BTRAPS(0x08)
		BTRAPS(0x10) BTRAPS(0x18)
		BTRAP(0x20) BTRAP(0x21) BTRAP(0x22) BTRAP(0x23)
		CLEAN_WINDOW ! 24-27
		BTRAPS(0x28)
		BTRAPS(0x30) BTRAPS(0x38)
#if 0
		BTRAP(0x40) BTRAP(0x41) BTRAP(0x42) BTRAP(0x43)
tl0_irq4:	TRAP_IRQ(handler_irq, 4)
tl0_irq5:	TRAP_IRQ(handler_irq, 5)  TRAP_IRQ(handler_irq, 6)
tl0_irq7:	TRAP_IRQ(handler_irq, 7)  TRAP_IRQ(handler_irq, 8)
tl0_irq9:	TRAP_IRQ(handler_irq, 9)  TRAP_IRQ(handler_irq, 10)
tl0_irq11:	TRAP_IRQ(handler_irq, 11) TRAP_IRQ(handler_irq, 12)
tl0_irq13:	TRAP_IRQ(handler_irq, 13)
tl0_irq14:	TRAP_IRQ(handler_irq, 14)
tl0_irq15:	TRAP_IRQ(handler_irq, 15)
#else
		BTRAPS(0x40) BTRAPS(0x48)
#endif
		BTRAPS(0x50) BTRAPS(0x58)
		BTRAPS(0x60) BTRAPS(0x68)
		BTRAPS(0x70) BTRAPS(0x78)
tl0_s0n:	SPILL_0_NORMAL
tl0_s1n:	SPILL_1_NORMAL
tl0_s2n:	SPILL_2_NORMAL
tl0_s3n:	SPILL_3_NORMAL
tl0_s4n:	SPILL_4_NORMAL
tl0_s5n:	SPILL_5_NORMAL
tl0_s6n:	SPILL_6_NORMAL
tl0_s7n:	SPILL_7_NORMAL
tl0_s0o:	SPILL_0_OTHER
tl0_s1o:	SPILL_1_OTHER
tl0_s2o:	SPILL_2_OTHER
tl0_s3o:	SPILL_3_OTHER
tl0_s4o:	SPILL_4_OTHER
tl0_s5o:	SPILL_5_OTHER
tl0_s6o:	SPILL_6_OTHER
tl0_s7o:	SPILL_7_OTHER
tl0_f0n:	FILL_0_NORMAL
tl0_f1n:	FILL_1_NORMAL
tl0_f2n:	FILL_2_NORMAL
tl0_f3n:	FILL_3_NORMAL
tl0_f4n:	FILL_4_NORMAL
tl0_f5n:	FILL_5_NORMAL
tl0_f6n:	FILL_6_NORMAL
tl0_f7n:	FILL_7_NORMAL
tl0_f0o:	FILL_0_OTHER
tl0_f1o:	FILL_1_OTHER
tl0_f2o:	FILL_2_OTHER
tl0_f3o:	FILL_3_OTHER
tl0_f4o:	FILL_4_OTHER
tl0_f5o:	FILL_5_OTHER
tl0_f6o:	FILL_6_OTHER
tl0_f7o:	FILL_7_OTHER
tl0_resv100:	BTRAPS(0x100) BTRAPS(0x108)
tl0_resv110:	BTRAPS(0x110) BTRAPS(0x118)
tl0_resv120:	BTRAPS(0x120) BTRAPS(0x128)
tl0_resv130:	BTRAPS(0x130) BTRAPS(0x138)
tl0_resv140:	BTRAPS(0x140) BTRAPS(0x148)
tl0_resv150:	BTRAPS(0x150) BTRAPS(0x158)
tl0_resv160:	BTRAPS(0x160) BTRAPS(0x168)
tl0_resv170:	BTRAPS(0x170) BTRAPS(0x178)
tl0_resv180:	BTRAPS(0x180) BTRAPS(0x188)
tl0_resv190:	BTRAPS(0x190) BTRAPS(0x198)
tl0_resv1a0:	BTRAPS(0x1a0) BTRAPS(0x1a8)
tl0_resv1b0:	BTRAPS(0x1b0) BTRAPS(0x1b8)
tl0_resv1c0:	BTRAPS(0x1c0) BTRAPS(0x1c8)
tl0_resv1d0:	BTRAPS(0x1d0) BTRAPS(0x1d8)
tl0_resv1e0:	BTRAPS(0x1e0) BTRAPS(0x1e8)
tl0_resv1f0:	BTRAPS(0x1f0) BTRAPS(0x1f8)

#undef BTRAPS
#define BTRAPS(x) BTRAPTL1(x) BTRAPTL1(x+1) BTRAPTL1(x+2) BTRAPTL1(x+3) BTRAPTL1(x+4) BTRAPTL1(x+5) BTRAPTL1(x+6) BTRAPTL1(x+7)

sparc64_ttable_tl1:
		BTRAPS(0x00) BTRAPS(0x08)
		BTRAPS(0x10) BTRAPS(0x18)
		BTRAPTL1(0x20) BTRAPTL1(0x21) BTRAPTL1(0x22) BTRAPTL1(0x23)
		CLEAN_WINDOW ! 24-27
		BTRAPS(0x28)
		BTRAPS(0x30) BTRAPS(0x38)
#if 0
		BTRAPTL1(0x40) BTRAPTL1(0x41) BTRAPTL1(0x42) BTRAPTL1(0x43)
tl1_irq4:	TRAP_IRQ(handler_irq, 4)
tl1_irq5:	TRAP_IRQ(handler_irq, 5)  TRAP_IRQ(handler_irq, 6)
tl1_irq7:	TRAP_IRQ(handler_irq, 7)  TRAP_IRQ(handler_irq, 8)
tl1_irq9:	TRAP_IRQ(handler_irq, 9)  TRAP_IRQ(handler_irq, 10)
tl1_irq11:	TRAP_IRQ(handler_irq, 11) TRAP_IRQ(handler_irq, 12)
tl1_irq13:	TRAP_IRQ(handler_irq, 13)
tl1_irq14:	TRAP_IRQ(handler_irq, 14)
tl1_irq15:	TRAP_IRQ(handler_irq, 15)
#else
		BTRAPS(0x40) BTRAPS(0x48)
#endif
		BTRAPS(0x50) BTRAPS(0x58)
		BTRAPS(0x60) BTRAPS(0x68)
		BTRAPS(0x70) BTRAPS(0x78)
tl1_s0n:	SPILL_0_NORMAL
tl1_s1n:	SPILL_1_NORMAL
tl1_s2n:	SPILL_2_NORMAL
tl1_s3n:	SPILL_3_NORMAL
tl1_s4n:	SPILL_4_NORMAL
tl1_s5n:	SPILL_5_NORMAL
tl1_s6n:	SPILL_6_NORMAL
tl1_s7n:	SPILL_7_NORMAL
tl1_s0o:	SPILL_0_OTHER
tl1_s1o:	SPILL_1_OTHER
tl1_s2o:	SPILL_2_OTHER
tl1_s3o:	SPILL_3_OTHER
tl1_s4o:	SPILL_4_OTHER
tl1_s5o:	SPILL_5_OTHER
tl1_s6o:	SPILL_6_OTHER
tl1_s7o:	SPILL_7_OTHER
tl1_f0n:	FILL_0_NORMAL
tl1_f1n:	FILL_1_NORMAL
tl1_f2n:	FILL_2_NORMAL
tl1_f3n:	FILL_3_NORMAL
tl1_f4n:	FILL_4_NORMAL
tl1_f5n:	FILL_5_NORMAL
tl1_f6n:	FILL_6_NORMAL
tl1_f7n:	FILL_7_NORMAL
tl1_f0o:	FILL_0_OTHER
tl1_f1o:	FILL_1_OTHER
tl1_f2o:	FILL_2_OTHER
tl1_f3o:	FILL_3_OTHER
tl1_f4o:	FILL_4_OTHER
tl1_f5o:	FILL_5_OTHER
tl1_f6o:	FILL_6_OTHER
tl1_f7o:	FILL_7_OTHER
tl1_resv100:	BTRAPS(0x100) BTRAPS(0x108)
tl1_resv110:	BTRAPS(0x110) BTRAPS(0x118)
tl1_resv120:	BTRAPS(0x120) BTRAPS(0x128)
tl1_resv130:	BTRAPS(0x130) BTRAPS(0x138)
tl1_resv140:	BTRAPS(0x140) BTRAPS(0x148)
tl1_resv150:	BTRAPS(0x150) BTRAPS(0x158)
tl1_resv160:	BTRAPS(0x160) BTRAPS(0x168)
tl1_resv170:	BTRAPS(0x170) BTRAPS(0x178)
tl1_resv180:	BTRAPS(0x180) BTRAPS(0x188)
tl1_resv190:	BTRAPS(0x190) BTRAPS(0x198)
tl1_resv1a0:	BTRAPS(0x1a0) BTRAPS(0x1a8)
tl1_resv1b0:	BTRAPS(0x1b0) BTRAPS(0x1b8)
tl1_resv1c0:	BTRAPS(0x1c0) BTRAPS(0x1c8)
tl1_resv1d0:	BTRAPS(0x1d0) BTRAPS(0x1d8)
tl1_resv1e0:	BTRAPS(0x1e0) BTRAPS(0x1e8)
tl1_resv1f0:	BTRAPS(0x1f0) BTRAPS(0x1f8)

spill_fixup:
spill_fixup_dax:
spill_fixup_mna:
fill_fixup:
fill_fixup_dax:
fill_fixup_mna:
__divide_error:
bug:
        /* Dump the exception and its context */
        ! Set up CPU state
        ! Don't change the global register set or we lose %g1 (exception level)
        rdpr    %pstate, %g2
        or      %g2, PSTATE_PRIV, %g2
        wrpr    %g2, %pstate
        wr      %g0, 0, %fprs
        wrpr    %g0, 0x0, %tl

        ! Disable I/D MMUs and caches
        stxa    %g0, [%g0] ASI_LSU_CONTROL

        ! Extract NWINDOWS from %ver
        rdpr    %ver, %g2
        and     %g2, 0xf, %g2
        wrpr    %g2, 0, %cleanwin
        wrpr    %g2, 0, %cansave
        wrpr    %g0, 0, %canrestore
        wrpr    %g0, 0, %otherwin
        wrpr    %g0, 0, %wstate

        call    dump_exception
         nop
_forever:
        /* Loop forever */
        b       _forever                                  ;
         nop
outstr:
        /* void outstr (unsigned long port, const unsigned char *str);
         * Writes a string on an IO port.
         */
1:      ldub    [%o1], %o3
        cmp     %o3, 0
        be      2f
         nop
        stba    %o3, [%o0] ASI_BP
        b       1b
         inc    %o1
2:      retl
         nop

outdigit:
        /* void outdigit (unsigned long port, uint8_t digit);
         * Dumps a single digit on serial port.
         */
        add     %o1, '0', %o1
        retl
         stba   %o1, [%o0] ASI_BP

outhex:
        /* void outhex (unsigned long port, uint64_t value);
         * Dumps a 64 bits hex number on serial port
         */
        mov     %o1, %o2
        set     60, %o3
        srlx    %o2, %o3, %o1
1:      and     %o1, 0xf, %o1
        cmp     %o1, 9
        bgt     2f
         nop
        b       3f
         add    %o1, '0', %o1
2:      add     %o1, 'a' - 10, %o1
3:      stba    %o1, [%o0] ASI_BP
        subcc   %o3, 4, %o3
        bge     1b
         srlx   %o2, %o3, %o1
        retl
         nop

        /* void dump_exception ();
         *
         * Dump a message when catching an exception
         */
dump_exception:
        setx    SER_ADDR, %o3, %o0
        setx    (_BUG_message_0), %o3, %o1
        call    outstr
         nop

        call    outhex
         mov    %g1, %o1

        setx    (_BUG_message_1), %o3, %o1
        call    outstr
         nop

        call    outhex
         rdpr   %tpc, %o1

        setx    (_BUG_message_2), %o3, %o1
        call    outstr
         nop

        call    outhex
         rdpr   %tnpc, %o1

        setx    (_BUG_message_3), %o3, %o1
        call    outstr
         nop

        retl
         nop

//#include "winfixup.S"

        .section .rodata
_BUG_message_0:
        .string "Unhandled Exception 0x"
_BUG_message_1:
        .string "\nPC = 0x"
_BUG_message_2:
        .string " NPC = 0x"
_BUG_message_3:
        .string "\nStopping execution\n"
