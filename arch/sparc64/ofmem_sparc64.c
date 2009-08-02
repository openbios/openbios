/*
 *	<ofmem_sparc64.c>
 *
 *	OF Memory manager
 *
 *   Copyright (C) 1999-2004 Samuel Rydh (samuel@ibrium.se)
 *   Copyright (C) 2004 Stefan Reinauer
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "libc/string.h"
#include "ofmem_sparc64.h"
#include "spitfire.h"

#define OF_MALLOC_BASE		((char*)OFMEM + ALIGN_SIZE(sizeof(ofmem_t), 8))

#define MEMSIZE ((128 + 256 + 512) * 1024)
static union {
	char memory[MEMSIZE];
	ofmem_t ofmem;
} s_ofmem_data;

#define OFMEM      (&s_ofmem_data.ofmem)
#define TOP_OF_RAM (s_ofmem_data.memory + MEMSIZE)

translation_t **g_ofmem_translations = &s_ofmem_data.ofmem.trans;

static ucell get_heap_top( void )
{
	return (ucell)TOP_OF_RAM;
}

static inline size_t ALIGN_SIZE(size_t x, size_t a)
{
    return (x + a - 1) & ~(a-1);
}

ofmem_t* ofmem_arch_get_private(void)
{
	return OFMEM;
}

void* ofmem_arch_get_malloc_base(void)
{
	return OF_MALLOC_BASE;
}

ucell ofmem_arch_get_heap_top(void)
{
	return get_heap_top();
}

ucell ofmem_arch_get_virt_top(void)
{
	return (ucell)TOP_OF_RAM;
}

/************************************************************************/
/* misc                                                                 */
/************************************************************************/

ucell ofmem_arch_default_translation_mode( ucell phys )
{
	/* Writable, cacheable */
	/* not privileged and not locked */
	return SPITFIRE_TTE_CP | SPITFIRE_TTE_CV | SPITFIRE_TTE_WRITABLE;
}




/************************************************************************/
/* init / cleanup                                                       */
/************************************************************************/

extern uint64_t qemu_mem_size;

static int remap_page_range( ucell phys, ucell virt, ucell size, ucell mode )
{
	ofmem_map_page_range(phys, virt, size, mode);
	if (!(mode & SPITFIRE_TTE_LOCKED)) {
		OFMEM_TRACE("remap_page_range clearing translation " FMT_ucellx
				" -> " FMT_ucellx " " FMT_ucellx " mode " FMT_ucellx "\n",
				virt, phys, size, mode );
		ofmem_arch_unmap_pages(virt, size);
	}
	return 0;
}

void ofmem_init( void )
{
	memset(&s_ofmem_data, 0, sizeof(s_ofmem_data));
	s_ofmem_data.ofmem.ramsize = qemu_mem_size;

	/* inherit translations set up by entry.S */
	ofmem_walk_boot_map(remap_page_range);
}

