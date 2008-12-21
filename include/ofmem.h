/*
 *   Creation Date: <1999/11/16 00:47:06 samuel>
 *   Time-stamp: <2003/10/18 13:28:14 samuel>
 *
 *	<ofmem.h>
 *
 *
 *
 *   Copyright (C) 1999, 2002 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#ifndef _H_OFMEM
#define _H_OFMEM

extern void	ofmem_cleanup( void );
extern void	ofmem_init( void );

extern ulong 	ofmem_claim( ulong addr, ulong size, ulong align );
extern ulong 	ofmem_claim_phys( ulong mphys, ulong size, ulong align );
extern ulong 	ofmem_claim_virt( ulong mvirt, ulong size, ulong align );

extern int 	ofmem_map( ulong phys, ulong virt, ulong size, int mode );

extern void  	ofmem_release( ulong virt, ulong size );
extern ulong 	ofmem_translate( ulong virt, int *ret_mode );

#endif   /* _H_OFMEM */
