/*
 *                     OpenBIOS - free your system!
 *                           ( detokenizer )
 *
 *  dictionary.c - dictionary initialization and functions.
 *
 *  This program is part of a free implementation of the IEEE 1275-1994
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 Stefan Reinauer, <stepan@openbios.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "detok.h"

typedef struct token {
	u8  *name;
	u16 fcode;
	struct token *next;
} token_t;

static char *fcerror="ferror";
token_t *dictionary=NULL;

char *lookup_token(u16 number)
{
	token_t *curr;
	
	for (curr=dictionary; curr!=NULL; curr=curr->next)
		if (curr->fcode==number)
			break;

	if (curr)
		return curr->name;

	return fcerror;
}

int add_token(u16 number, char *name)
{
	token_t *curr;

	curr=malloc(sizeof(token_t));
	if(!curr) {
		printf("Out of memory while adding token.\n");
		exit(-ENOMEM);
	}

	curr->next=dictionary;
	curr->fcode=number;
	curr->name=name;

	dictionary=curr;
	return 0;
}

void init_dictionary(void) 
{
	add_token( 0x000, "end0" );
	add_token( 0x010, "b(lit)" );
	add_token( 0x011, "b(')" );
	add_token( 0x012, "b(\")" );
	add_token( 0x013, "bbranch" );
	add_token( 0x014, "b?branch" );
	add_token( 0x015, "b(loop)" );
	add_token( 0x016, "b(+loop)" );
	add_token( 0x017, "b(do)" );
	add_token( 0x018, "b(?do)" );
	add_token( 0x019, "i" );
	add_token( 0x01a, "j" );
	add_token( 0x01b, "b(leave)" );
	add_token( 0x01c, "b(of)" );
	add_token( 0x01d, "execute" );
	add_token( 0x01e, "+" );
	add_token( 0x01f, "-" );
	add_token( 0x020, "*" );
	add_token( 0x021, "/" );
	add_token( 0x022, "mod" );
	add_token( 0x023, "and" );
	add_token( 0x024, "or" );
	add_token( 0x025, "xor" );
	add_token( 0x026, "invert" );
	add_token( 0x027, "lshift" );
	add_token( 0x028, "rshift" );
	add_token( 0x029, ">>a" );
	add_token( 0x02a, "/mod" );
	add_token( 0x02b, "u/mod" );
	add_token( 0x02c, "negate" );
	add_token( 0x02d, "abs" );
	add_token( 0x02e, "min" );
	add_token( 0x02f, "max" );
	add_token( 0x030, ">r" );
	add_token( 0x031, "r>" );
	add_token( 0x032, "r@" );
	add_token( 0x033, "exit" );
	add_token( 0x034, "0=" );
	add_token( 0x035, "0<>" );
	add_token( 0x036, "0<" );
	add_token( 0x037, "0<=" );
	add_token( 0x038, "0>" );
	add_token( 0x039, "0>=" );
	add_token( 0x03a, "<" );
	add_token( 0x03b, ">" );
	add_token( 0x03c, "=" );
	add_token( 0x03d, "<>" );
	add_token( 0x03e, "u>" );
	add_token( 0x03f, "u<=" );
	add_token( 0x040, "u<" );
	add_token( 0x041, "u>=" );
	add_token( 0x042, ">=" );
	add_token( 0x043, "<=" );
	add_token( 0x044, "between" );
	add_token( 0x045, "within" );
	add_token( 0x046, "drop" );
	add_token( 0x047, "dup" );
	add_token( 0x048, "over" );
	add_token( 0x049, "swap" );
	add_token( 0x04A, "rot" );
	add_token( 0x04b, "-rot" );
	add_token( 0x04c, "tuck" );
	add_token( 0x04d, "nip" );
	add_token( 0x04e, "pick" );
	add_token( 0x04f, "roll" );
	add_token( 0x050, "?dup" );
	add_token( 0x051, "depth" );
	add_token( 0x052, "2drop" );
	add_token( 0x053, "2dup" );
	add_token( 0x054, "2over" );
	add_token( 0x055, "2swap" );
	add_token( 0x056, "2rot" );
	add_token( 0x057, "2/" );
	add_token( 0x058, "u2/" );
	add_token( 0x059, "2*" );
	add_token( 0x05a, "/c" );
	add_token( 0x05b, "/w" );
	add_token( 0x05c, "/l" );
	add_token( 0x05d, "/n" );
	add_token( 0x05e, "ca+" );
	add_token( 0x05f, "wa+" );
	add_token( 0x060, "la+" );
	add_token( 0x061, "na+" );
	add_token( 0x062, "char+" );
	add_token( 0x063, "wa1+" );
	add_token( 0x064, "la1+" );
	add_token( 0x065, "cell+" );
	add_token( 0x066, "chars" );
	add_token( 0x067, "/w*" );
	add_token( 0x068, "/l*" );
	add_token( 0x069, "cells" );
	add_token( 0x06a, "on" );
	add_token( 0x06b, "off" );
	add_token( 0x06c, "+!" );
	add_token( 0x06d, "@" );
	add_token( 0x06e, "l@" );
	add_token( 0x06f, "w@" );
	add_token( 0x070, "<w@" );
	add_token( 0x071, "c@" );
	add_token( 0x072, "!" );
	add_token( 0x073, "l!" );
	add_token( 0x074, "w!" );
	add_token( 0x075, "c!" );
	add_token( 0x076, "2@" );
	add_token( 0x077, "2!" );
	add_token( 0x078, "move" );
	add_token( 0x079, "fill" );
	add_token( 0x07a, "comp" );
	add_token( 0x07b, "noop" );
	add_token( 0x07c, "lwsplit" );
	add_token( 0x07d, "wljoin" );
	add_token( 0x07e, "lbsplit" );
	add_token( 0x07f, "bljoin" );
	add_token( 0x080, "wbflip" );
	add_token( 0x081, "upc" );
	add_token( 0x082, "lcc" );
	add_token( 0x083, "pack" );
	add_token( 0x084, "count" );
	add_token( 0x085, "body>" );
	add_token( 0x086, ">body" );
	add_token( 0x087, "fcode-revision" );
	add_token( 0x088, "span" );
	add_token( 0x089, "unloop" );
	add_token( 0x08a, "expect" );
	add_token( 0x08b, "alloc-mem" );
	add_token( 0x08c, "free-mem" );
	add_token( 0x08d, "key?" );
	add_token( 0x08e, "key" );
	add_token( 0x08f, "emit" );
	add_token( 0x090, "type" );
	add_token( 0x091, "(cr" );
	add_token( 0x092, "cr" );
	add_token( 0x093, "#out" );
	add_token( 0x094, "#line" );
	add_token( 0x095, "hold" );
	add_token( 0x096, "<#" );
	add_token( 0x097, "u#>" );
	add_token( 0x098, "sign" );
	add_token( 0x099, "u#" );
	add_token( 0x09a, "u#s" );
	add_token( 0x09b, "u." );
	add_token( 0x09c, "u.r" );
	add_token( 0x09d, "." );
	add_token( 0x09e, ".r" );
	add_token( 0x09f, ".s" );
	add_token( 0x0a0, "base" );
	add_token( 0x0a1, "convert" );
	add_token( 0x0a2, "$number" );
	add_token( 0x0a3, "digit" );
	add_token( 0x0a4, "-1" );
	add_token( 0x0a5, "0" );
	add_token( 0x0a6, "1" );
	add_token( 0x0a7, "2" );
	add_token( 0x0a8, "3" );
	add_token( 0x0a9, "bl" );
	add_token( 0x0aa, "bs" );
	add_token( 0x0ab, "bell" );
	add_token( 0x0ac, "bounds" );
	add_token( 0x0ad, "here" );
	add_token( 0x0ae, "aligned" );
	add_token( 0x0af, "wbsplit" );
	add_token( 0x0b0, "bwjoin" );
	add_token( 0x0b1, "b(<mark)" );
	add_token( 0x0b2, "b(>resolve)" );
	add_token( 0x0b3, "set-token-table" );
	add_token( 0x0b4, "set-table" );
	add_token( 0x0b5, "new-token" );
	add_token( 0x0b6, "named-token" );
	add_token( 0x0b7, "b(:)" );
	add_token( 0x0b8, "b(value)" );
	add_token( 0x0b9, "b(variable)" );
	add_token( 0x0ba, "b(constant)" );
	add_token( 0x0bb, "b(create)" );
	add_token( 0x0bc, "b(defer)" );
	add_token( 0x0bd, "b(buffer:)" );
	add_token( 0x0be, "b(field)" );
	add_token( 0x0bf, "b(code)" );
	add_token( 0x0c0, "instance" );
	add_token( 0x0c2, "b(;)" );
	add_token( 0x0c3, "b(to)" );
	add_token( 0x0c4, "b(case)" );
	add_token( 0x0c5, "b(endcase)" );
	add_token( 0x0c6, "b(endof)" );
	add_token( 0x0c7, "#" );
	add_token( 0x0c8, "#s" );
	add_token( 0x0c9, "#>" );
	add_token( 0x0ca, "external-token" );
	add_token( 0x0cb, "$find" );
	add_token( 0x0cc, "offset16" );
	add_token( 0x0cd, "evaluate" );
	add_token( 0x0d0, "c," );
	add_token( 0x0d1, "w," );
	add_token( 0x0d2, "l," );
	add_token( 0x0d3, "," );
	add_token( 0x0d4, "um*" );
	add_token( 0x0d5, "um/mod" );
	add_token( 0x0d8, "d+" );
	add_token( 0x0d9, "d-" );
	add_token( 0x0da, "get-token" );
	add_token( 0x0db, "set-token" );
	add_token( 0x0dc, "state" );
	add_token( 0x0dd, "compile" );
	add_token( 0x0de, "behavior" );
	add_token( 0x0f0, "start0" );
	add_token( 0x0f1, "start1" );
	add_token( 0x0f2, "start2" );
	add_token( 0x0f3, "start4" );
	add_token( 0x0fc, "ferror" );
	add_token( 0x0fd, "version1" );
	add_token( 0x0fe, "4-byte-id" );
	add_token( 0x0ff, "end1" );
	add_token( 0x101, "dma-alloc" );
	add_token( 0x102, "my-address" );
	add_token( 0x103, "my-space" );
	add_token( 0x104, "memmap" );
	add_token( 0x105, "free-virtual" );
	add_token( 0x106, ">physical" );
	add_token( 0x10f, "my-params" );
	add_token( 0x110, "property" );
	add_token( 0x111, "encode-int" );
	add_token( 0x112, "encode+" );
	add_token( 0x113, "encode-phys" );
	add_token( 0x114, "encode-string" );
	add_token( 0x115, "encode-bytes" );
	add_token( 0x116, "reg" );
	add_token( 0x117, "intr" );
	add_token( 0x118, "driver" );
	add_token( 0x119, "model" );
	add_token( 0x11a, "device-type" );
	add_token( 0x11b, "parse-2int" );
	add_token( 0x11c, "is-install" );
	add_token( 0x11d, "is-remove" );
	add_token( 0x11e, "is-selftest" );
	add_token( 0x11f, "new-device" );
	add_token( 0x120, "diagnostic-mode?" );
	add_token( 0x121, "display-status" );
	add_token( 0x122, "memory-test-issue" );
	add_token( 0x123, "group-code" );
	add_token( 0x124, "mask" );
	add_token( 0x125, "get-msecs" );
	add_token( 0x126, "ms" );
	add_token( 0x127, "finish-device" );
	add_token( 0x128, "decode-phys" );
	add_token( 0x12b, "interpose" );
	add_token( 0x130, "map-low" );
	add_token( 0x131, "sbus-intr>cpu" );
	add_token( 0x150, "#lines" );
	add_token( 0x151, "#columns" );
	add_token( 0x152, "line#" );
	add_token( 0x153, "column#" );
	add_token( 0x154, "inverse?" );
	add_token( 0x155, "inverse-screen?" );
	add_token( 0x156, "frame-buffer-busy?" );
	add_token( 0x157, "draw-character" );
	add_token( 0x158, "reset-screen" );
	add_token( 0x159, "toggle-cursor" );
	add_token( 0x15a, "erase-screen" );
	add_token( 0x15b, "blink-screen" );
	add_token( 0x15c, "invert-screen" );
	add_token( 0x15d, "insert-characters" );
	add_token( 0x15e, "delete-characters" );
	add_token( 0x15f, "insert-lines" );
	add_token( 0x160, "delete-lines" );
	add_token( 0x161, "draw-logo" );
	add_token( 0x162, "frame-buffer-adr" );
	add_token( 0x163, "screen-height" );
	add_token( 0x164, "screen-width" );
	add_token( 0x165, "window-top" );
	add_token( 0x166, "window-left" );
	add_token( 0x16a, "default-font" );
	add_token( 0x16b, "set-font" );
	add_token( 0x16c, "char-height" );
	add_token( 0x16d, "char-width" );
	add_token( 0x16e, ">font" );
	add_token( 0x16f, "fontbytes" );
	add_token( 0x170, "fb1-draw-character" );
	add_token( 0x171, "fb1-reset-screen" );
	add_token( 0x172, "fb1-toggle-cursor" );
	add_token( 0x173, "fb1-erase-screen" );
	add_token( 0x174, "fb1-blink-screen" );
	add_token( 0x175, "fb1-invert-screen" );
	add_token( 0x176, "fb1-insert-characters" );
	add_token( 0x177, "fb1-delete-characters" );
	add_token( 0x178, "fb1-insert-lines" );
	add_token( 0x179, "fb1-delete-lines" );
	add_token( 0x17a, "fb1-draw-logo" );
	add_token( 0x17b, "fb1-install" );
	add_token( 0x17c, "fb1-slide-up" );
	add_token( 0x180, "fb8-draw-character" );
	add_token( 0x181, "fb8-reset-screen" );
	add_token( 0x182, "fb8-toggle-cursor" );
	add_token( 0x183, "fb8-erase-screen" );
	add_token( 0x184, "fb8-blink-screen" );
	add_token( 0x185, "fb8-invert-screen" );
	add_token( 0x186, "fb8-insert-characters" );
	add_token( 0x187, "fb8-delete-characters" );
	add_token( 0x188, "fb8-insert-lines" );
	add_token( 0x189, "fb8-delete-lines" );
	add_token( 0x18a, "fb8-draw-logo" );
	add_token( 0x18b, "fb8-install" );
	add_token( 0x1a0, "return-buffer" );
	add_token( 0x1a1, "xmit-packet" );
	add_token( 0x1a2, "poll-packet" );
	add_token( 0x1a4, "mac-address" );
	add_token( 0x201, "device-name" );
	add_token( 0x202, "my-args" );
	add_token( 0x203, "my-self" );
	add_token( 0x204, "find-package" );
	add_token( 0x205, "open-package" );
	add_token( 0x206, "close-package" );
	add_token( 0x207, "find-method" );
	add_token( 0x208, "call-package" );
	add_token( 0x209, "$call-parent" );
	add_token( 0x20a, "my-package" );
	add_token( 0x20b, "ihandle>phandle" );
	add_token( 0x20d, "my-unit" );
	add_token( 0x20e, "$call-method" );
	add_token( 0x20f, "$open-package" );
	add_token( 0x210, "processor-type" );
	add_token( 0x211, "firmware-version" );
	add_token( 0x212, "fcode-version" );
	add_token( 0x213, "alarm" );
	add_token( 0x214, "(is-user-word)" );
	add_token( 0x215, "suspend-fcode" );
	add_token( 0x216, "abort" );
	add_token( 0x217, "catch" );
	add_token( 0x218, "throw" );
	add_token( 0x219, "user-abort" );
	add_token( 0x21a, "get-my-property" );
	add_token( 0x21b, "decode-int" );
	add_token( 0x21c, "decode-string" );
	add_token( 0x21d, "get-inherited-property" );
	add_token( 0x21e, "delete-property" );
	add_token( 0x21f, "get-package-property" );
	add_token( 0x220, "cpeek" );
	add_token( 0x221, "wpeek" );
	add_token( 0x222, "lpeek" );
	add_token( 0x223, "cpoke" );
	add_token( 0x224, "wpoke" );
	add_token( 0x225, "lpoke" );
	add_token( 0x226, "lwflip" );
	add_token( 0x227, "lbflip" );
	add_token( 0x228, "lbflips" );
	add_token( 0x229, "adr-mask" );
	add_token( 0x230, "rb@" );
	add_token( 0x231, "rb!" );
	add_token( 0x232, "rw@" );
	add_token( 0x233, "rw!" );
	add_token( 0x234, "rl@" );
	add_token( 0x235, "rl!" );
	add_token( 0x236, "wbflips" );
	add_token( 0x237, "lwflips" );
	add_token( 0x238, "probe" );
	add_token( 0x239, "probe-virtual" );
	add_token( 0x23b, "child" );
	add_token( 0x23c, "peer" );
	add_token( 0x23d, "next-property" );
	add_token( 0x23e, "byte-load" );
	add_token( 0x23f, "set-args" );
	add_token( 0x240, "left-parse-string" );

	/* FCodes from 64bit extension addendum */
	add_token( 0x22e, "rx@" );
	add_token( 0x22f, "rx!" );
	add_token( 0x241, "bxjoin" );
	add_token( 0x242, "<l@" );
	add_token( 0x243, "lxjoin" );
	add_token( 0x244, "wxjoin" );
	add_token( 0x245, "x," );
	add_token( 0x246, "x@" );
	add_token( 0x247, "x!" );
	add_token( 0x248, "/x" );
	add_token( 0x249, "/x*" );
	add_token( 0x24a, "xa+" );
	add_token( 0x24b, "xa1+" );
	add_token( 0x24c, "xbflip" );
	add_token( 0x24d, "xbflips" );
	add_token( 0x24e, "xbsplit" );
	add_token( 0x24f, "xlflip" );
	add_token( 0x250, "xlflips" );
	add_token( 0x251, "xlsplit" );
	add_token( 0x252, "xwflip" );
	add_token( 0x253, "xwflips" );
	add_token( 0x254, "xwsplit" );
}
