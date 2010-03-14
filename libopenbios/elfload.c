/* Mac-on-Linux ELF loader

   Copyright (C) 2001-2003 Samuel Rydh

   adapted from yaboot

   Copyright (C) 1999 Benjamin Herrenschmidt

   portions based on poof

   Copyright (C) 1999 Marius Vollmer

   portions based on quik

   Copyright (C) 1996 Paul Mackerras.

   Because this program is derived from the corresponding file in the
   silo-0.64 distribution, it is also

   Copyright (C) 1996 Pete A. Zaitcev
   		 1996 Maurizio Plaza
   		 1996 David S. Miller
   		 1996 Miguel de Icaza
   		 1996 Jakub Jelinek

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "openbios/config.h"
#include "libopenbios/bindings.h"
#include "libopenbios/elfload.h"
#include "libc/diskio.h"
#include "openbios/elf.h"

#define DEBUG		0
#define MAX_HEADERS	32
#define BS		0x100	/* smallest step used when looking for the ELF header */


int
find_elf( int fd )
{
	int size, offs;

	seek_io( fd, -1 );
	size = tell( fd );
	if( size > 0x10000 )
		size = 0x10000;

	for( offs=0; offs < size; offs+= BS )
		if( is_elf(fd, offs) )
			return offs;
	return -1;
}

int
is_elf( int fd, int offs )
{
	Elf_ehdr e;

	seek_io( fd, offs );
	if( read_io(fd, &e, sizeof(e)) != sizeof(e) ) {
		printk("\nCan't read ELF image header\n");
		return 0;
	}

	return (e.e_ident[EI_MAG0]  == ELFMAG0	    &&
		e.e_ident[EI_MAG1]  == ELFMAG1	    &&
		e.e_ident[EI_MAG2]  == ELFMAG2	    &&
		e.e_ident[EI_MAG3]  == ELFMAG3	    &&
		e.e_ident[EI_CLASS] == ARCH_ELF_CLASS  &&
		e.e_ident[EI_DATA]  == ARCH_ELF_DATA   &&
		e.e_type            == ET_EXEC	    &&
		ARCH_ELF_MACHINE_OK(e.e_machine));
}

Elf_phdr *
elf_readhdrs( int fd, int offs, Elf_ehdr *e )
{
	int size;
	Elf_phdr *ph;

	if( !is_elf(fd, offs) ) {
		printk("Not an ELF image\n");
		return NULL;
	}

	seek_io( fd, offs );
	if( read_io(fd, e, sizeof(*e)) != sizeof(*e) ) {
		printk("\nCan't read ELF image header\n");
		return NULL;
	}

#if DEBUG
	printk("ELF header:\n");
	printk(" e.e_type    = %d\n", (int)e->e_type);
	printk(" e.e_machine = %d\n", (int)e->e_machine);
	printk(" e.e_version = %d\n", (int)e->e_version);
	printk(" e.e_entry   = 0x%08x\n", (int)e->e_entry);
	printk(" e.e_phoff   = 0x%08x\n", (int)e->e_phoff);
	printk(" e.e_shoff   = 0x%08x\n", (int)e->e_shoff);
	printk(" e.e_flags   = %d\n", (int)e->e_flags);
	printk(" e.e_ehsize  = 0x%08x\n", (int)e->e_ehsize);
	printk(" e.e_phentsize = 0x%08x\n", (int)e->e_phentsize);
	printk(" e.e_phnum   = %d\n", (int)e->e_phnum);
#endif
	if (e->e_phnum > MAX_HEADERS) {
		printk ("elfload: too many program headers (MAX_HEADERS)\n");
		return NULL;
	}

	size = sizeof(Elf_phdr) * e->e_phnum;
	if( !(ph=(Elf_phdr *)malloc(size)) ) {
		printk("malloc error\n");
		return NULL;
	}

	/* Now, we read the section header */
	seek_io( fd, offs+e->e_phoff );
	if( read_io(fd, (char*)ph, size) != size ) {
		printk("read error");
		free( ph );
		return NULL;
	}
	return ph;
}
