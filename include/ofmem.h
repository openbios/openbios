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

extern ucell 	ofmem_claim( ucell addr, ucell size, ucell align );
extern ucell 	ofmem_claim_phys( ucell mphys, ucell size, ucell align );
extern ucell 	ofmem_claim_virt( ucell mvirt, ucell size, ucell align );

extern int 	ofmem_map( ucell phys, ucell virt, ucell size, ucell mode );

extern void  	ofmem_release( ucell virt, ucell size );
extern ucell 	ofmem_translate( ucell virt, ucell *ret_mode );

#ifdef CONFIG_PPC
#define PAGE_SHIFT   12

ulong get_ram_size( void );
ulong get_ram_top( void );
ulong get_ram_bottom( void );
void ofmem_register( phandle_t ph );
#elif defined(CONFIG_SPARC32)
#define PAGE_SHIFT   12

/* arch/sparc32/lib.c */
struct mem;
extern struct mem cdvmem;

void mem_init(struct mem *t, char *begin, char *limit);
void *mem_alloc(struct mem *t, int size, int align);
int map_page(unsigned long va, uint64_t epa, int type);
void *map_io(uint64_t pa, int size);
#endif

#ifdef PAGE_SHIFT
#define PAGE_SIZE    (1 << PAGE_SHIFT)
#define PAGE_MASK    (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(addr)  (((addr) + PAGE_SIZE - 1) & PAGE_MASK)
#endif

#if defined(CONFIG_DEBUG_OFMEM)
# define OFMEM_TRACE(fmt, ...) do { printk("OFMEM: " fmt, ## __VA_ARGS__); } while (0)
#else
# define OFMEM_TRACE(fmt, ...) do {} while(0)
#endif

#endif   /* _H_OFMEM */
