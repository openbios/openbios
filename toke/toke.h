/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  toke.h - tokenizer base macros.  
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

#ifndef _H_TOKE
#define _H_TOKE

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define s16 short
#define bool int
#define TRUE (-1)
#define FALSE (0)
#define ANSI_ONLY

extern void	init_scanner( void );
extern void	exit_scanner( void );

extern void	init_dictionary( void );
extern void	init_macros( void );
extern void	tokenize( void );

extern u16	lookup_token(char *name);
extern u16	lookup_fword(char *name);
extern char	*lookup_macro(char *name);
extern int	add_token(u16 number, char *name);

extern int	add_macro(char *name, char *alias);

#endif   /* _H_TOKE */
