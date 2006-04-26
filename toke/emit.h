/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  emit.h - prototypes for fcode emitters
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

#ifndef _H_EMIT
#define _H_EMIT

int emit_byte(u8 data);
int emit_fcode(u16 tok);
int emit_token(const char *name);
int emit_num32(u32 num);
int emit_num16(u16 num);
int emit_offset(s16 offs);
s16 receive_offset(void);
int emit_string(u8 *string, unsigned int cnt);
int emit_fcodehdr(void);
int finish_fcodehdr(void);
int emit_pcihdr(u16 vid, u16 did, u32 classid);
int finish_pcihdr(void);
void finish_headers(void);

#endif   /* _H_EMIT */
