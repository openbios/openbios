/*
 *
 *       <elf-loader.c>
 *
 *       ELF file loader
 *
 *   Copyright (C) 2009 Laurent Vivier (Laurent@vivier.eu)
 *
 *   Some parts Copyright (C) 2002, 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "modules.h"
#include "ofmem.h"

#include "openbios/elf.h"
#include "asm/elf.h"

/* TODO: manage ELF notes section */

//#define DEBUG_ELF

#ifdef DEBUG_ELF
#define DPRINTF(fmt, args...) \
    do { printk("%s: " fmt, __func__ , ##args); } while (0)
#else
#define DPRINTF(fmt, args...) \
    do { } while (0)
#endif

DECLARE_NODE(elf_loader, INSTALL_OPEN, 0, "+/packages/elf-loader" );

#ifdef CONFIG_PPC
extern void             flush_icache_range( char *start, char *stop );
#endif

static void
elf_loader_init_program( void *dummy )
{
	char *base;
	int i;
	Elf_ehdr *ehdr;
	Elf_phdr *phdr;
	size_t size;
	char *addr;

	feval("load-base");
	base = (char*)POP();

	ehdr = (Elf_ehdr *)base;
	phdr = (Elf_phdr *)(base + ehdr->e_phoff);

	for (i = 0; i < ehdr->e_phnum; i++) {
		DPRINTF("filesz: %08lX memsz: %08lX p_offset: %08lX "
                        "p_vaddr %08lX\n",
			(ulong)phdr[i].p_filesz, (ulong)phdr[i].p_memsz,
			(ulong)phdr[i].p_offset, (ulong)phdr[i].p_vaddr );

		size = MIN(phdr[i].p_filesz, phdr[i].p_memsz);
		if (!size)
			continue;
		if( ofmem_claim( phdr[i].p_vaddr, phdr[i].p_memsz, 0 ) == -1 ) {
                        printk("Claim failed!\n");
			return;
		}
		addr = (char*)phdr[i].p_vaddr;
		memcpy(addr, base + phdr[i].p_offset, size);
#ifdef CONFIG_PPC
		flush_icache_range( addr, addr + size );
#endif
	}
	/* FIXME: should initialize saved-program-state. */
	PUSH(ehdr->e_entry);
	feval("elf-entry !");
}

NODE_METHODS( elf_loader ) = {
	{ "init-program", elf_loader_init_program },
};

void elf_loader_init( void )
{
	REGISTER_NODE( elf_loader );
}
