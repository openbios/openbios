/*
 *   Creation Date: <2001/05/05 16:44:17 samuel>
 *   Time-stamp: <2003/10/22 23:18:42 samuel>
 *
 *	<elfload.h>
 *
 *	Elf loader
 *
 *   Copyright (C) 2001, 2003 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#ifndef _H_ELFLOAD
#define _H_ELFLOAD

#include "arch/common/elf.h"
#include "asm/elf.h"

extern int		is_elf( int fd, int offs );
extern int		find_elf( int fd );

extern Elf32_Phdr *	elf_readhdrs( int fd, int offs, Elf32_Ehdr *e );


#endif   /* _H_ELFLOAD */
