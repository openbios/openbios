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

#include "kernel/stack.h"

typedef struct alloc_desc {
	struct alloc_desc 	*next;
	ucell			size;			/* size (including) this struct */
} alloc_desc_t;

typedef struct mem_range {
	struct mem_range	*next;
	ucell			start;
	ucell			size;
} range_t;

typedef struct trans {
	struct trans		*next;
	ucell			virt;			/* chain is sorted by virt */
	ucell			size;
	ucell			phys;
	ucell			mode;
} translation_t;

/* ofmem private data */
typedef struct {
	ucell			ramsize;
	char 			*next_malloc;
	alloc_desc_t	*mfree;		/* list of free malloc blocks */

	range_t			*phys_range;
	range_t			*virt_range;

	translation_t	*trans;		/* this is really a translation_t */
} ofmem_t;

/* TODO: temporary migration interface */
extern ofmem_t* ofmem_arch_get_private(void);
extern void*    ofmem_arch_get_malloc_base(void);
extern ucell    ofmem_arch_get_heap_top(void);
extern ucell    ofmem_arch_get_virt_top(void);
extern ucell    ofmem_arch_default_translation_mode( ucell phys );
extern void     ofmem_arch_early_map_pages(ucell phys, ucell virt, ucell size,
                                           ucell mode);
extern void     ofmem_arch_unmap_pages(ucell virt, ucell size);
/* sparc64 uses this method */
extern int      ofmem_map_page_range( ucell phys, ucell virt, ucell size,
                                      ucell mode );

/* malloc interface */
extern void* ofmem_malloc( size_t size );
extern void  ofmem_free( void *ptr );
extern void* ofmem_realloc( void *ptr, size_t size );

/* ofmem_common.c */

extern void	ofmem_cleanup( void );
extern void	ofmem_init( void );

/*
 * register /memory and /virtual-memory handles
 * ofmem module will update "available" and "translations" properties
 * using these handles
 *
 * to disable updating /memory properties  pass zero memory handle
 */
extern void ofmem_register( phandle_t ph_memory, phandle_t ph_mmu );

extern ucell ofmem_claim( ucell addr, ucell size, ucell align );
extern ucell ofmem_claim_phys( ucell mphys, ucell size, ucell align );
extern ucell ofmem_claim_virt( ucell mvirt, ucell size, ucell align );

extern int   ofmem_map( ucell phys, ucell virt, ucell size, ucell mode );
extern int   ofmem_unmap( ucell virt, ucell size );

extern void  ofmem_release_phys( ucell phys, ucell size );
extern void  ofmem_release_virt( ucell virt, ucell size );
extern ucell ofmem_translate( ucell virt, ucell *ret_mode );

#ifdef CONFIG_PPC
#define PAGE_SHIFT   12

ulong get_ram_top( void );
ulong get_ram_bottom( void );

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
