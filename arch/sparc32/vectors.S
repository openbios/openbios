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

#define __ASSEMBLY
#include "psr.h"
#include "asi.h"
#define ASI_BP ASI_M_BYPASS
#define SER_ADDR 0x71100004

        .section ".text.vectors", "ax"
        .align 16384
/* Sparc32 trap table */
        .globl trap_table, t_zero, t_wovf, t_wunf, __divide_error
trap_table:

#define WINDOW_SPILL \
        rd %psr, %l0; rd %wim, %l3; b spill_window_entry; nop;

#define WINDOW_FILL \
        rd %psr, %l0; rd %wim, %l3; b fill_window_entry; nop;

#define BTRAP(lvl) ba bug; mov lvl, %g1; nop; nop;
#define BTRAPS(x) BTRAP(x) BTRAP(x+1) BTRAP(x+2) BTRAP(x+3) BTRAP(x+4) BTRAP(x+5) BTRAP(x+6) BTRAP(x+7)
t_zero:         b entry; nop; nop; nop;
                BTRAP(0x1) BTRAP(0x2) BTRAP(0x3) BTRAP(0x4)
t_wovf:         WINDOW_SPILL                        /* Window Overflow               */
t_wunf:         WINDOW_FILL                         /* Window Underflow              */
                BTRAP(0x7)
                BTRAPS(0x8)
#if 0
                BAD_TRAP(0x10)
t_irq1:         TRAP_ENTRY_INTERRUPT(1)             /* IRQ Software/SBUS Level 1     */
t_irq2:         TRAP_ENTRY_INTERRUPT(2)             /* IRQ SBUS Level 2              */
t_irq3:         TRAP_ENTRY_INTERRUPT(3)             /* IRQ SCSI/DMA/SBUS Level 3     */
t_irq4:         TRAP_ENTRY_INTERRUPT(4)             /* IRQ Software Level 4          */
t_irq5:         TRAP_ENTRY_INTERRUPT(5)             /* IRQ SBUS/Ethernet Level 5     */
t_irq6:         TRAP_ENTRY_INTERRUPT(6)             /* IRQ Software Level 6          */
t_irq7:         TRAP_ENTRY_INTERRUPT(7)             /* IRQ Video/SBUS Level 5        */
t_irq8:         TRAP_ENTRY_INTERRUPT(8)             /* IRQ SBUS Level 6              */
t_irq9:         TRAP_ENTRY_INTERRUPT(9)             /* IRQ SBUS Level 7              */
t_irq10:        TRAP_ENTRY_INTERRUPT(10)            /* IRQ Timer #1 (one we use)     */
t_irq11:        TRAP_ENTRY_INTERRUPT(11)            /* IRQ Floppy Intr.              */
t_irq12:        TRAP_ENTRY_INTERRUPT(12)            /* IRQ Zilog serial chip         */
t_irq13:        TRAP_ENTRY_INTERRUPT(13)            /* IRQ Audio Intr.               */
t_irq14:        TRAP_ENTRY_INTERRUPT(14)            /* IRQ Timer #2                  */
t_nmi:          BAD_TRAP(0x1f)                      /* Level 15 (NMI)                */
#else
                BTRAPS(0x10) BTRAPS(0x18)
#endif
                BTRAPS(0x20) BTRAPS(0x28)
                BTRAPS(0x30) BTRAPS(0x38)
                BTRAPS(0x40) BTRAPS(0x48)
                BTRAPS(0x50) BTRAPS(0x58)
                BTRAPS(0x60) BTRAPS(0x68)
                BTRAPS(0x70) BTRAPS(0x78)
                BTRAPS(0x80) BTRAPS(0x88)
                BTRAPS(0x90) BTRAPS(0x98)
                BTRAPS(0xa0) BTRAPS(0xa8)
                BTRAPS(0xb0) BTRAPS(0xb8)
                BTRAPS(0xc0) BTRAPS(0xc8)
                BTRAPS(0xd0) BTRAPS(0xd8)
                BTRAPS(0xe0) BTRAPS(0xe8)
                BTRAPS(0xf0) BTRAPS(0xf8)

__divide_error:
bug:
        /* Dump the exception and its context */
        ! Set up CPU state
        rd      %psr, %g2
        andn    %g2, PSR_ET, %g2
        wr      %g2, %psr
        ! Disable mmu
        call    dump_exception
         sta    %g0, [%g0] ASI_M_MMUREGS
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
        /* void outhex (unsigned long port, uint32_t value);
         * Dumps a 32 bits hex number on serial port
         */
        mov     %o1, %o2
        set     28, %o3
        srl     %o2, %o3, %o1
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
         srl    %o2, %o3, %o1
        retl
         nop

        /* void dump_exception ();
         *
         * Dump a message when catching an exception
         */
dump_exception:
        set     SER_ADDR + 2, %o0
        set     (_BUG_message_0), %o1
        call    outstr
         nop

        call    outhex
         mov    %g1, %o1

        set     (_BUG_message_1), %o1
        call    outstr
         nop

        call    outhex
         mov    %l1, %o1

        set     (_BUG_message_2), %o1
        call    outstr
         nop

        call    outhex
         mov    %l2, %o1

        set     (_BUG_message_3), %o1
        call    outstr
         nop

        retl
         nop
/* Register window handlers */
#include "wof.S"
#include "wuf.S"

        .section .rodata
_BUG_message_0:
        .string "Unhandled Exception 0x"
_BUG_message_1:
        .string "\nPC = 0x"
_BUG_message_2:
        .string " NPC = 0x"
_BUG_message_3:
        .string "\nStopping execution\n"
