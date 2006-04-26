/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  stream.h - prototypes for streaming functions. 
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

#ifndef _H_STREAM
#define _H_STREAM

int init_stream( const char *name );
int close_stream(void);
int init_output( const char *inname, const char *outname );
int close_output(void);

extern unsigned int 	lineno;
extern char		*iname;

#endif   /* _H_STREAM */
