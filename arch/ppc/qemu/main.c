/*
 *   Creation Date: <2002/10/02 22:24:24 samuel>
 *   Time-stamp: <2004/03/27 01:57:55 samuel>
 *
 *	<main.c>
 *
 *
 *
 *   Copyright (C) 2002, 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */


#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/elfload.h"
#include "openbios/nvram.h"
#include "libc/diskio.h"
#include "libc/vsprintf.h"
#include "kernel.h"
#include "ofmem.h"

//#define DEBUG_ELF

#ifdef DEBUG_ELF
#define ELF_DPRINTF(fmt, args...) \
do { printk("ELF - %s: " fmt, __func__ , ##args); } while (0)
#else
#define ELF_DPRINTF(fmt, args...) do { } while (0)
#endif

static void
transfer_control_to_elf( ulong elf_entry )
{
	ELF_DPRINTF("Starting ELF boot loader\n");
        call_elf( elf_entry );

	fatal_error("call_elf returned unexpectedly\n");
}

static int
load_elf_rom( ulong *elf_entry, int fd )
{
	int i, lszz_offs, elf_offs;
        char *addr;
	Elf_ehdr ehdr;
	Elf_phdr *phdr;
	size_t s;

	ELF_DPRINTF("Loading '%s' from '%s'\n", get_file_path(fd),
	       get_volume_name(fd) );

	/* the ELF-image (usually) starts at offset 0x4000 */
	if( (elf_offs=find_elf(fd)) < 0 ) {
		ELF_DPRINTF("----> %s is not an ELF image\n", buf );
		exit(1);
	}
	if( !(phdr=elf_readhdrs(fd, elf_offs, &ehdr)) )
		fatal_error("elf_readhdrs failed\n");

	*elf_entry = ehdr.e_entry;

	/* load segments. Compressed ROM-image assumed to be located immediately
	 * after the last segment */
	lszz_offs = elf_offs;
	for( i=0; i<ehdr.e_phnum; i++ ) {
		/* p_memsz, p_flags */
		s = MIN( phdr[i].p_filesz, phdr[i].p_memsz );
		seek_io( fd, elf_offs + phdr[i].p_offset );

		ELF_DPRINTF("filesz: %08lX memsz: %08lX p_offset: %08lX p_vaddr %08lX\n",
		   phdr[i].p_filesz, phdr[i].p_memsz, phdr[i].p_offset,
		   phdr[i].p_vaddr );

		if( phdr[i].p_vaddr != phdr[i].p_paddr )
			ELF_DPRINTF("WARNING: ELF segment virtual addr != physical addr\n");
		lszz_offs = MAX( lszz_offs, elf_offs + phdr[i].p_offset + phdr[i].p_filesz );
		if( !s )
			continue;
		if( ofmem_claim( phdr[i].p_vaddr, phdr[i].p_memsz, 0 ) == -1 )
			fatal_error("Claim failed!\n");

		addr = (char*)phdr[i].p_vaddr;
		if( read_io(fd, addr, s) != s )
			fatal_error("read failed\n");

		flush_icache_range( addr, addr+s );

		ELF_DPRINTF("ELF ROM-section loaded at %08lX (size %08lX)\n",
			    (ulong)phdr[i].p_vaddr, (ulong)phdr[i].p_memsz );
	}
	free( phdr );
	return lszz_offs;
}

static void
encode_bootpath( const char *spec, const char *args )
{
	phandle_t chosen_ph = find_dev("/chosen");
	set_property( chosen_ph, "bootpath", spec, strlen(spec)+1 );
	set_property( chosen_ph, "bootargs", args, strlen(args)+1 );
}

/************************************************************************/
/*	qemu booting							*/
/************************************************************************/

static void
yaboot_startup( void )
{
	const char *paths[] = { "hd:2,\\ofclient", "hd:2,\\yaboot", NULL };
	const char *args[] = { "", "conf=hd:2,\\yaboot.conf", NULL };
        ulong elf_entry;
	int i, fd;

	for( i=0; paths[i]; i++ ) {
		if( (fd=open_io(paths[i])) == -1 )
			continue;
                (void) load_elf_rom( &elf_entry, fd );
		close_io( fd );
		encode_bootpath( paths[i], args[i] );

		update_nvram();
		ELF_DPRINTF("Transfering control to %s %s\n",
			    paths[i], args[i]);
                transfer_control_to_elf( elf_entry );
		/* won't come here */
	}
	printk("*** Boot failure! No secondary bootloader specified ***\n");
}


/************************************************************************/
/*	entry								*/
/************************************************************************/

void
boot( void )
{
	fword("update-chosen");
	yaboot_startup();
}
