/*
 * <adbkbd.c>
 *
 * Open Hack'Ware BIOS ADB keyboard support, ported to OpenBIOS
 * 
 *  Copyright (c) 2005 Jocelyn Mayer
 *  Copyright (c) 2005 Stefan Reinauer
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License V2
 *   as published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 */


#include "openbios/config.h"
#include "openbios/bindings.h"
#include "asm/types.h"
#include "adb.h"
#include "kbd.h"
#include "cuda.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"

//#define DEBUG_ADB 1

#ifdef DEBUG_ADB
#define ADB_DPRINTF(fmt, args...) \
do { printk("ADB - %s: " fmt, __func__ , ##args); } while (0)
#else
#define ADB_DPRINTF(fmt, args...) do { } while (0)
#endif

DECLARE_UNNAMED_NODE( keyboard, INSTALL_OPEN, sizeof(int));

static void
keyboard_open(int *idx)
{
	RET(-1);
}

static void
keyboard_close(int *idx)
{
}

static void keyboard_read(void);

NODE_METHODS( keyboard ) = {
	{ "open",		keyboard_open		},
	{ "close",		keyboard_close		},
	{ "read",               keyboard_read		},
};

/* ADB US keyboard translation map
 * XXX: for now, only shift modifier is defined
 */


static keymap_t ADB_kbd_us[] = {
    /* 0x00 */
    { KBD_SH_CAPS, { 0x61, 0x41,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x73, 0x53,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x64, 0x44,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x66, 0x46,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x68, 0x48,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x67, 0x47,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x7A, 0x5A,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x78, 0x58,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    /* 0x08 */
    { KBD_SH_CAPS, { 0x63, 0x43,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x76, 0x56,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x60, 0x40,   -1,   -1,   -1,   -1,   -1,   -1, /* ? */
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x62, 0x42,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x71, 0x51,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x77, 0x57,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x65, 0x45,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x72, 0x52,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    /* 0x10 */
    { KBD_SH_CAPS, { 0x79, 0x59,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x74, 0x54,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x31, 0x21,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x32, 0x40,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x33, 0x23,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x34, 0x24,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x36, 0x5E,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x35, 0x25,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    /* 0x18 */
    { KBD_SH_CAPS, { 0x3D, 0x2B,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x39, 0x28,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x37, 0x26,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x2D, 0x5F,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x38, 0x2A,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x30, 0x29,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x5D, 0x7D,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x6F, 0x4F,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    /* 0x20 */
    { KBD_SH_CAPS, { 0x75, 0x55,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x5B, 0x7B,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x69, 0x49,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x70, 0x50,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_MOD_MAP(0x0D), },
    { KBD_SH_CAPS, { 0x6C, 0x4C,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x6A, 0x4A,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x27, 0x22,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    /* 0x28 */
    { KBD_SH_CAPS, { 0x6B, 0x4B,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x3B, 0x3A,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x5C, 0x7C,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x2C, 0x3C,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x2F, 0x3F,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x6E, 0x4E,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x6D, 0x4D,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_CAPS, { 0x2E, 0x3E,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    /* 0x30 : tab */
    { KBD_MOD_MAP(0x09), },
    /* 0x31 : space */
    { KBD_MOD_MAP(0x20), },
    /* 0x32 : '<' '>' */
    { KBD_SH_CAPS, { 0x3C, 0x3E,   -1,   -1,   -1,   -1,   -1,   -1, /* ? */
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    /* 0x33 : backspace */
    { KBD_MOD_MAP(0x08), },
    { KBD_MAP_NONE, },
    /* 0x35 : ESC */
    { KBD_MOD_MAP(0x1B), },
    /* 0x36 : control */
    { KBD_MOD_MAP_LCTRL, },
    /* 0x37 : command */
    { KBD_MOD_MAP_LCMD,   },
    /* 0x38 : left shift */
    { KBD_MOD_MAP_LSHIFT, },
    /* 0x39 : caps-lock */
    { KBD_MOD_MAP_CAPS,   },
    /* 0x3A : option */
    { KBD_MOD_MAP_LOPT,   },
    /* 0x3B : left */
    { KBD_MAP_NONE, },
    /* 0x3C : right */
    { KBD_MAP_NONE, },
    /* 0x3D : down */
    { KBD_MAP_NONE, },
    /* 0x3E : up */
    { KBD_MAP_NONE, },
    { KBD_MAP_NONE, },
    /* 0x40 */
    { KBD_MAP_NONE, },
    { KBD_SH_NUML, { 0x7F, 0x2E,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_MAP_NONE, },
    { KBD_SH_NONE, { 0x2A, 0x2A,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_MAP_NONE, },
    { KBD_SH_NONE, { 0x2B, 0x2B,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_MAP_NONE, },
    { KBD_MOD_MAP(0x7F), },
    /* 0x48 */
    { KBD_MAP_NONE, },
    { KBD_MAP_NONE, },
    { KBD_MAP_NONE, },
    { KBD_SH_NONE, { 0x2F, 0x2F,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_MOD_MAP(0x0D), },
    { KBD_MAP_NONE, },
    { KBD_SH_NONE, { 0x2D, 0x2D,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_MAP_NONE, },
    /* 0x50 */
    { KBD_MAP_NONE, },
    { KBD_SH_NONE, { 0x3D, 0x3D,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_NUML, {   -1, 0x30,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_NUML, {   -1, 0x31,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_NUML, {   -1, 0x32,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_NUML, {   -1, 0x33,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_NUML, {   -1, 0x34,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_NUML, {   -1, 0x35,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    /* 0x58 */
    { KBD_SH_NUML, {   -1, 0x36,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_NUML, {   -1, 0x37,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_MAP_NONE, },
    { KBD_SH_NUML, {   -1, 0x38,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_SH_NUML, {   -1, 0x39,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
                       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, }, },
    { KBD_MAP_NONE, },
    { KBD_MOD_MAP(0x2F), },
    { KBD_MAP_NONE, },
    /* 0x60 : F5 */
    { KBD_MAP_NONE, },
    /* 0x61 : F6 */
    { KBD_MAP_NONE, },
    /* 0x62 : F7 */
    { KBD_MAP_NONE, },
    /* 0x63 : F3 */
    { KBD_MAP_NONE, },
    /* 0x64 : F8 */
    { KBD_MAP_NONE, },
    /* 0x65 : F9 */
    { KBD_MAP_NONE, },
    { KBD_MAP_NONE, },
    /* 0x67 : F11 */
    { KBD_MAP_NONE, },
    /* 0x68 */
    { KBD_MAP_NONE, },
    /* 0x69 : F13 */
    { KBD_MAP_NONE, },
    { KBD_MAP_NONE, },
    /* 0x6B : F14 */
    { KBD_MAP_NONE, },
    { KBD_MAP_NONE, },
    /* 0x6D : F10 */
    { KBD_MAP_NONE, },
    { KBD_MAP_NONE, },
    /* 0x6F : F12 */
    { KBD_MAP_NONE, },
    /* 0x70 */
    { KBD_MAP_NONE, },
    /* 0x71 : F15 */
    { KBD_MAP_NONE, },
    /* 0x72 : help */
    { KBD_MAP_NONE, },
    /* 0x73 : home */
    { KBD_MAP_NONE, },
    /* 0x74 : page up */
    { KBD_MAP_NONE, },
    /* 0x75 : del */
    { KBD_MAP_NONE, },
    /* 0x76 : F4 */
    { KBD_MAP_NONE, },
    /* 0x77 : end */
    { KBD_MAP_NONE, },
    /* 0x78 : F2 */
    { KBD_MAP_NONE, },
    /* 0x79 : page down */
    { KBD_MAP_NONE, },
    /* 0x7A : F1 */
    { KBD_MAP_NONE, },
    /* 0x7B : right shift */
    { KBD_MOD_MAP_RSHIFT, },
    /* 0x7C : right option */
    { KBD_MOD_MAP_ROPT,   },
    /* 0x7D : right control */
    { KBD_MOD_MAP_RCTRL, },
    { KBD_MAP_NONE, },
    /* 0x7F : power */
    { KBD_MAP_NONE, },
};

typedef struct adb_kbd_t adb_kbd_t;
struct adb_kbd_t {
    kbd_t kbd;
    int next_key;
};

static adb_dev_t *my_adb_dev = NULL;

static int adb_kbd_read (void *private)
{
    uint8_t buffer[ADB_BUF_SIZE];
    adb_dev_t *dev = private;
    adb_kbd_t *kbd;
    int key;
    int ret;

    kbd = (void *)dev->state;
    /* Get saved state */
    ret = -1;
    for (key = -1; key == -1; ) {
        if (kbd->next_key != -1) {
            key = kbd->next_key;
            kbd->next_key = -1;
        } else {
            if (adb_reg_get(dev, 0, buffer) != 2)
		break;
            kbd->next_key = buffer[1] == 0xFF ? -1 : buffer[1];
            key = buffer[0];
        }
        ret = kbd_translate_key(&kbd->kbd, key & 0x7F, key >> 7);
        ADB_DPRINTF("Translated %d (%02x) into %d (%02x)\n",
                    key, key, ret, ret);
    }

    return ret;
}


void *adb_kbd_new (char *path, void *private)
{
	char buf[64];
	int props[1];
	phandle_t ph, aliases;
    adb_kbd_t *kbd;
    adb_dev_t *dev = private;
    kbd = (adb_kbd_t*)malloc(sizeof(adb_kbd_t));
    if (kbd != NULL) {
        memset(kbd, 0, sizeof(adb_kbd_t));
        kbd_set_keymap(&kbd->kbd, sizeof(ADB_kbd_us) / sizeof(keymap_t),
			ADB_kbd_us);
        kbd->next_key = -1;
	dev->state = (int32_t)kbd;
	my_adb_dev = dev;
    }

	sprintf(buf, "%s/keyboard", path);
	REGISTER_NAMED_NODE( keyboard, buf);

	ph = find_dev(buf);

	set_property(ph, "device_type", "keyboard", 9);
	props[0] = __cpu_to_be32(dev->addr);
	set_property(ph, "reg", (char *)&props, sizeof(props));

	aliases = find_dev("/aliases");
	set_property(aliases, "adb-keyboard", buf, strlen(buf) + 1);

    return kbd;
}

/* ( addr len -- actual ) */
static void keyboard_read(void)
{
	char *addr;
	int len, key, i;
	len=POP();
	addr=(char *)POP();
	
	for (i = 0; i < len; i++) {
		key = adb_kbd_read(my_adb_dev);
		if (key == -1 || key == -2)
			break;
		*addr++ = (char)key;
	}
	PUSH(i);
}

DECLARE_UNNAMED_NODE( mouse, INSTALL_OPEN, sizeof(int));

static void
mouse_open(int *idx)
{
	RET(-1);
}

static void
mouse_close(int *idx)
{
}

NODE_METHODS( mouse ) = {
	{ "open",		mouse_open		},
	{ "close",		mouse_close		},
};

void adb_mouse_new (char *path, void *private)
{
	char buf[64];
	int props[1];
	phandle_t ph, aliases;
	adb_dev_t *dev = private;

	sprintf(buf, "%s/mouse", path);
	REGISTER_NAMED_NODE( mouse, buf);

	ph = find_dev(buf);

	set_property(ph, "device_type", "mouse", 6);
	props[0] = __cpu_to_be32(dev->addr);
	set_property(ph, "reg", (char *)&props, sizeof(props));
	set_int_property(ph, "#buttons", 3);

	aliases = find_dev("/aliases");
	set_property(aliases, "adb-mouse", buf, strlen(buf) + 1);
}


int adb_cmd (adb_dev_t *dev, uint8_t cmd, uint8_t reg,
             uint8_t *buf, int len)
{
    uint8_t adb_send[ADB_BUF_SIZE], adb_rcv[ADB_BUF_SIZE];

    //ADB_DPRINTF("cmd: %d reg: %d len: %d\n", cmd, reg, len);
    if (dev->bus == NULL || dev->bus->req == NULL) {
        ADB_DPRINTF("ERROR: invalid bus !\n");
	for (;;);
    }
    /* Sanity checks */
    if (cmd != ADB_LISTEN && len != 0) {
        /* No buffer transmitted but for LISTEN command */
        ADB_DPRINTF("in buffer for cmd %d\n", cmd);
        return -1;
    }
    if (cmd == ADB_LISTEN && ((len < 2 || len > 8) || buf == NULL)) {
        /* Need a buffer with a regular register size for LISTEN command */
        ADB_DPRINTF("no/invalid buffer for ADB_LISTEN (%d)\n", len);
        return -1;
    }
    if ((cmd == ADB_TALK || cmd == ADB_LISTEN) && reg > 3) {
        /* Need a valid register number for LISTEN and TALK commands */
        ADB_DPRINTF("invalid reg for TALK/LISTEN command (%d %d)\n", cmd, reg);
        return -1;
    }
    switch (cmd) {
    case ADB_SEND_RESET:
        adb_send[0] = ADB_SEND_RESET;
        break;
    case ADB_FLUSH:
        adb_send[0] = (dev->addr << 4) | ADB_FLUSH;
        break;
    case ADB_LISTEN:
        memcpy(adb_send + 1, buf, len);
        /* No break here */
    case ADB_TALK:
        adb_send[0] = (dev->addr << 4) | cmd | reg;
        break;
    }
    memset(adb_rcv, 0, ADB_BUF_SIZE);
    len = (*dev->bus->req)(dev->bus->host, adb_send, len + 1, adb_rcv);
#ifdef DEBUG_ADB
    //printk("%x %x %x %x\n", adb_rcv[0], adb_rcv[1], adb_rcv[2], adb_rcv[3]);
#endif
    switch (len) {
    case 0:
        /* No data */
        break;
    case 2 ... 8:
        /* Register transmitted */
        if (buf != NULL)
            memcpy(buf, adb_rcv, len);
        break;
    default:
        /* Should never happen */
        //ADB_DPRINTF("Cmd %d returned %d bytes !\n", cmd, len);
        return -1;
    }
    //ADB_DPRINTF("retlen: %d\n", len);

    return len;
}
