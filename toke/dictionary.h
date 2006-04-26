/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  dictionary.h - tokens for control commands.
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

#define COLON		0x01
#define SEMICOLON	0x02
#define TOKENIZE	0x03
#define AGAIN		0x04
#define ALIAS		0x05
#define GETTOKEN 	0x06
#define ASCII		0x07
#define BEGIN		0x08
#define BUFFER		0x09
#define CASE		0x0a
#define CONST		0x0b
#define CONTROL		0x0c
#define CREATE		0x0d
#define DECIMAL		0x0e
#define DEFER		0x0f
#define CDO		0x10
#define DO		0x11
#define ELSE		0x12
#define ENDCASE		0x13
#define ENDOF		0x14
#define EXTERNAL	0x15
#define FIELD		0x16
#define HEADERLESS	0x17
#define HEADERS		0x18
#define HEX		0x19
#define IF		0x1a
#define CLEAVE		0x1b
#define LEAVE		0x1c
#define CLOOP		0x1d
#define LOOP		0x1e
#define OCTAL		0x1f
#define OF		0x20
#define REPEAT		0x21
#define THEN		0x22
#define TO		0x23
#define UNTIL		0x24
#define VALUE		0x25
#define VARIABLE	0x26	
#define WHILE		0x27
#define OFFSET16	0x28	
#define BEGINTOK	0x29	
#define EMITBYTE	0x2a	
#define ENDTOK		0x2b
#define FLOAD		0x2c
#define STRING		0x2d
#define PSTRING		0x2e
#define PBSTRING	0x2f
#define SSTRING		0x30
#define RECURSIVE	0x31
#define HEXVAL		0x32
#define DECVAL		0x33
#define OCTVAL		0x34

#define END0		0xdb
#define END1		0xdc
#define CHAR		0xdd
#define CCHAR		0xde
#define ABORTTXT	0xdf

#define NEXTFCODE	0xef

#define ENCODEFILE	0xf0

#define FCODE_V1	0xf1
#define FCODE_V3	0xf2
#define NOTLAST		0xf3
#define PCIREV		0xf4
#define PCIHDR		0xf5
#define PCIEND		0xf6
#define START0		0xf7
#define START1		0xf8
#define START2		0xf9
#define START4		0xfa
#define VERSION1	0xfb
#define FCODE_TIME	0xfc
#define FCODE_DATE	0xfd
#define FCODE_V2	0xfe
#define FCODE_END	0xff
