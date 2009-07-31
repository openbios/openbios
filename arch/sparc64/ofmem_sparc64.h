/*
 *	<ofmem_sparc64.h>
 *
 *	OF Memory manager
 *
 *   Copyright (C) 1999, 2002 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#ifndef _H_OFMEM_SPARC64
#define _H_OFMEM_SPARC64

#include "ofmem.h"

extern void* ofmem_malloc( size_t size );
extern void  ofmem_free( void *ptr );
extern void* ofmem_realloc( void *ptr, size_t size );

extern int ofmem_unmap(ucell virt, ucell size);

ulong get_ram_size( void );
ulong get_ram_top( void );
ulong get_ram_bottom( void );
extern void ofmem_register( phandle_t ph_memory, phandle_t ph_mmu );
extern void ofmem_map_pages(ucell phys, ucell virt, ucell size, ucell mode);
extern void ofmem_unmap_pages(ucell virt, ucell size);

typedef int (*translation_entry_cb)(ucell phys,	ucell virt,
									ucell size, ucell mode);

extern void ofmem_walk_boot_map(translation_entry_cb cb);

typedef struct alloc_desc {
	struct alloc_desc	*next;
	ucell				size;			/* size (including) this struct */
} alloc_desc_t;

typedef struct mem_range {
	struct mem_range	*next;
	ucell				start;
	ucell				size;
} range_t;

typedef struct trans {
	struct trans	*next;
	ucell			virt;		/* chain is sorted by virt */
	ucell			size;
	ucell			phys;
	ucell			mode;
} translation_t;

typedef struct {
	uint64_t		ramsize;
	char 			*next_malloc;
	alloc_desc_t	*mfree;		/* list of free malloc blocks */

	range_t			*phys_range;
	range_t			*virt_range;

	translation_t	*trans;		/* this is really a translation_t */
} ofmem_t;

extern translation_t **g_ofmem_translations;

#endif   /* _H_OFMEM_SPARC64 */
