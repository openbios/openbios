/*
 *   Creation Date: <2003/12/23 00:32:12 samuel>
 *   Time-stamp: <2003/12/28 14:47:02 samuel>
 *
 *	<modules.h>
 *
 *	Module initialization
 *
 *   Copyright (C) 2003 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#ifndef _H_MODULES
#define _H_MODULES

extern void	deblocker_init( void );
extern void	disklabel_init( void );
extern void	filesystem_init( void );
extern void	macparts_init( void );
extern void	pcparts_init( void );
extern void	sunparts_init( void );
extern void	cmdline_init( void );
extern void	elf_loader_init( void );
extern void	xcoff_loader_init( void );

#endif   /* _H_MODULES */
