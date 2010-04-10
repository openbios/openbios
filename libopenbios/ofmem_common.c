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

#include "config.h"
#include "libopenbios/bindings.h"
#include "libopenbios/ofmem.h"

/*
 * define OFMEM_FILL_RANGE to claim any unclaimed virtual and
 * physical memory in the range for ofmem_map
 *
 * TODO: remove this macro and wrapped code if not needed by implementations
 */
//#define OFMEM_FILL_RANGE


static inline size_t ALIGN_SIZE(size_t x, size_t a)
{
    return (x + a - 1) & ~(a-1);
}

static ucell get_ram_size( void )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	return ofmem->ramsize;
}

/************************************************************************/
/* debug                                                                */
/************************************************************************/

#if 0
static void
print_range( range_t *r, char *str )
{
	printk("--- Range %s ---\n", str );
	for( ; r; r=r->next )
		printk("%08lx - %08lx\n", r->start, r->start + r->size -1 );
	printk("\n");
}

static void
print_phys_range()
{
	print_range( ofmem.phys_range, "phys" );
}

static void
print_virt_range()
{
	print_range( ofmem.virt_range, "virt" );
}

static void
print_trans( void )
{
	translation_t *t = ofmem.trans;

	printk("--- Translations ---\n");
	for( ; t; t=t->next )
		printk("%08lx -> %08lx [size %lx]\n", t->virt, t->phys, t->size );
	printk("\n");
}
#endif

/************************************************************************/
/* OF private allocations                                               */
/************************************************************************/

void* ofmem_malloc( size_t size )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	alloc_desc_t *d, **pp;
	char *ret;
	ucell top;

	if( !size )
		return NULL;

	if( !ofmem->next_malloc )
		ofmem->next_malloc = (char*)ofmem_arch_get_malloc_base();

	size = ALIGN_SIZE(size + sizeof(alloc_desc_t), CONFIG_OFMEM_MALLOC_ALIGN);

	/* look in the freelist */
	for( pp=&ofmem->mfree; *pp && (**pp).size < size; pp = &(**pp).next ) {
	}

	/* waste at most 4K by taking an entry from the freelist */
	if( *pp && (**pp).size < size + 0x1000 ) {
		ret = (char*)*pp + sizeof(alloc_desc_t);
		memset( ret, 0, (**pp).size - sizeof(alloc_desc_t) );
		*pp = (**pp).next;
		return ret;
	}

	top = ofmem_arch_get_heap_top();

	if( (ucell)ofmem->next_malloc + size > top ) {
		OFMEM_TRACE("out of malloc memory (%x)!\n", size );
		return NULL;
	}

	d = (alloc_desc_t*) ofmem->next_malloc;
	ofmem->next_malloc += size;

	d->next = NULL;
	d->size = size;

	ret = (char*)d + sizeof(alloc_desc_t);
	memset( ret, 0, size - sizeof(alloc_desc_t) );

	return ret;
}

void ofmem_free( void *ptr )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	alloc_desc_t **pp, *d;

	/* it is legal to free NULL pointers (size zero allocations) */
	if( !ptr )
		return;

	d = (alloc_desc_t*)((char *)ptr - sizeof(alloc_desc_t));
	d->next = ofmem->mfree;

	/* insert in the (sorted) freelist */
	for( pp=&ofmem->mfree; *pp && (**pp).size < d->size ; pp = &(**pp).next ) {
	}

	d->next = *pp;
	*pp = d;
}

void* ofmem_realloc( void *ptr, size_t size )
{
	alloc_desc_t *d = (alloc_desc_t*)((char *)ptr - sizeof(alloc_desc_t));
	char *p;

	if( !ptr )
		return malloc( size );
	if( !size ) {
		free( ptr );
		return NULL;
	}
	p = malloc( size );
	memcpy( p, ptr, MIN(d->size - sizeof(alloc_desc_t),size) );
	free( ptr );
	return p;
}


/************************************************************************/
/* "translations" and "available" property tracking                     */
/************************************************************************/

static phandle_t s_phandle_memory = 0;
static phandle_t s_phandle_mmu = 0;

static void ofmem_update_mmu_translations( void )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	translation_t *t;
	int ncells;
	ucell *props;

	if (s_phandle_mmu == 0)
		return;

	for( t = ofmem->trans, ncells = 0; t ; t=t->next, ncells++ ) {
	}

	props = malloc(ncells * sizeof(ucell) * 3);

	if (props == NULL)
		return;

	for( t = ofmem->trans, ncells = 0 ; t ; t=t->next ) {
		props[ncells++] = t->virt;
		props[ncells++] = t->size;
		props[ncells++] = t->mode;
	}

	set_property(s_phandle_mmu, "translations",
			(char*)props, ncells * sizeof(props[0]));

	free(props);
}

static void ofmem_update_memory_available( phandle_t ph, range_t *range,
		u64 top_address )
{
	range_t *r;
	int ncells;
	ucell *props;

	ucell start, size;

	if (s_phandle_memory == 0)
		return;

	/* count phys_range list entries */
	for( r = range, ncells = 0; r ; r=r->next, ncells++ ) {
	}

	/* inverse of phys_range list could take 2 more cells for the tail */
	props = malloc((ncells+1) * sizeof(ucell) * 2);

	if (props == NULL) {
		/* out of memory! */
		return;
	}

	start = 0;
	ncells = 0;

	for (r = range; r; r=r->next) {
		if (r->start >= top_address) {
			break;
		}

		size = r->start - start;
		if (size) {
			props[ncells++] = start;
			props[ncells++] = size;
		}
		start = r->start + r->size;
	}

	/* tail */
	if (start < top_address) {
		props[ncells++] = start;
		props[ncells++] = top_address - start;
	}

	set_property(ph, "available",
			(char*)props, ncells * sizeof(props[0]));

	free(props);
}

static void ofmem_update_translations( void )
{
	ofmem_t *ofmem = ofmem_arch_get_private();

	ofmem_update_memory_available(s_phandle_memory,
			ofmem->phys_range, get_ram_size());
	ofmem_update_memory_available(s_phandle_mmu,
			ofmem->virt_range, -1ULL);
	ofmem_update_mmu_translations();
}


/************************************************************************/
/* client interface                                                     */
/************************************************************************/

static int is_free( ucell ea, ucell size, range_t *r )
{
	if( size == 0 )
		return 1;
	for( ; r ; r=r->next ) {
		if( r->start + r->size - 1 >= ea && r->start <= ea )
			return 0;
		if( r->start >= ea && r->start <= ea + size - 1 )
			return 0;
	}
	return 1;
}

static void add_entry_( ucell ea, ucell size, range_t **r )
{
	range_t *nr;

	for( ; *r && (**r).start < ea; r=&(**r).next ) {
	}

	nr = (range_t*)malloc( sizeof(range_t) );
	nr->next = *r;
	nr->start = ea;
	nr->size = size;
	*r = nr;
}

static int add_entry( ucell ea, ucell size, range_t **r )
{
	if( !is_free( ea, size, *r ) ) {
		OFMEM_TRACE("add_entry: range not free!\n");
		return -1;
	}
	add_entry_( ea, size, r );
	return 0;
}

#if defined(OFMEM_FILL_RANGE)
static void join_ranges( range_t **rr )
{
	range_t *n, *r = *rr;
	while( r ) {
		if( !(n=r->next) )
			break;

		if( r->start + r->size - 1 >= n->start -1 ) {
			int s = n->size + (n->start - r->start - r->size);
			if( s > 0 )
				r->size += s;
			r->next = n->next;
			free( n );
			continue;
		}
		r=r->next;
	}
}

static void fill_range( ucell ea, ucell size, range_t **rr )
{
	add_entry_( ea, size, rr );
	join_ranges( rr );
}
#endif

static ucell find_area( ucell align, ucell size, range_t *r,
		ucell min, ucell max, int reverse )
{
	ucell base = min;
	range_t *r2;

	if( (align & (align-1)) ) {
		OFMEM_TRACE("bad alignment " FMT_ucell "\n", align);
		align = 0x1000;
	}
	if( !align )
		align = 0x1000;

	base = reverse ? max - size : min;
	r2 = reverse ? NULL : r;

	for( ;; ) {
		if( !reverse ) {
			base = (base + align - 1) & ~(align-1);
			if( base < min )
				base = min;
			if( base + size - 1 >= max -1 )
				break;
		} else {
			if( base > max - size )
				base = max - size;
			base -= base & (align-1);
		}
		if( is_free( base, size, r ) )
			return base;

		if( !reverse ) {
			if( !r2 )
				break;
			base = r2->start + r2->size;
			r2 = r2->next;
		} else {
			range_t *rp;

			for( rp=r; rp && rp->next != r2 ; rp=rp->next ) {
			}

			r2 = rp;
			if( !r2 )
				break;
			base = r2->start - size;
		}
	}
	return -1;
}

static ucell ofmem_claim_phys_( ucell phys, ucell size, ucell align,
		ucell min, ucell max, int reverse )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	if( !align ) {
		if( !is_free( phys, size, ofmem->phys_range ) ) {
			OFMEM_TRACE("Non-free physical memory claimed!\n");
			return -1;
		}
		add_entry( phys, size, &ofmem->phys_range );
		return phys;
	}
	phys = find_area( align, size, ofmem->phys_range, min, max, reverse );
	if( phys == -1 ) {
		OFMEM_TRACE("ofmem_claim_phys - out of space\n");
		return -1;
	}
	add_entry( phys, size, &ofmem->phys_range );

	return phys;
}

/* if align != 0, phys is ignored. Returns -1 on error */
ucell ofmem_claim_phys( ucell phys, ucell size, ucell align )
{
    OFMEM_TRACE("ofmem_claim phys=" FMT_ucellx " size=" FMT_ucellx
                " align=" FMT_ucellx "\n",
                phys, size, align);

	return ofmem_claim_phys_( phys, size, align, 0, get_ram_size(), 0 );
}

static ucell ofmem_claim_virt_( ucell virt, ucell size, ucell align,
		ucell min, ucell max, int reverse )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	if( !align ) {
		if( !is_free( virt, size, ofmem->virt_range ) ) {
			OFMEM_TRACE("Non-free virtual memory claimed!\n");
			return -1;
		}
		add_entry( virt, size, &ofmem->virt_range );
		return virt;
	}

	virt = find_area( align, size, ofmem->virt_range, min, max, reverse );
	if( virt == -1 ) {
		OFMEM_TRACE("ofmem_claim_virt - out of space\n");
		return -1;
	}
	add_entry( virt, size, &ofmem->virt_range );
	return virt;
}

ucell ofmem_claim_virt( ucell virt, ucell size, ucell align )
{
    OFMEM_TRACE("ofmem_claim_virt virt=" FMT_ucellx " size=" FMT_ucellx
                " align=" FMT_ucellx "\n",
                virt, size, align);

	/* printk("+ ofmem_claim virt %08lx %lx %ld\n", virt, size, align ); */
	return ofmem_claim_virt_( virt, size, align,
			get_ram_size(), ofmem_arch_get_virt_top(), 0 );
}


/* allocate both physical and virtual space and add a translation */
ucell ofmem_claim( ucell addr, ucell size, ucell align )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	ucell virt, phys;
	ucell offs = addr & 0xfff;

	/* printk("+ ofmem_claim %08lx %lx %ld\n", addr, size, align ); */
	virt = phys = 0;
	if( !align ) {
		if( is_free(addr, size, ofmem->virt_range) &&
		    is_free(addr, size, ofmem->phys_range) ) {
			ofmem_claim_phys_( addr, size, 0, 0, 0, 0 );
			ofmem_claim_virt_( addr, size, 0, 0, 0, 0 );
			virt = phys = addr;
		} else {
			OFMEM_TRACE("**** ofmem_claim failure ***!\n");
			return -1;
		}
	} else {
		if( align < 0x1000 )
			align = 0x1000;
		phys = ofmem_claim_phys_( addr, size, align, 0, get_ram_size(), 1 /* reverse */ );
		virt = ofmem_claim_virt_( addr, size, align, 0, get_ram_size(), 1 /* reverse */ );
		if( phys == -1 || virt == -1 ) {
			OFMEM_TRACE("ofmem_claim failed\n");
			return -1;
		}
		/* printk("...phys = %08lX, virt = %08lX, size = %08lX\n", phys, virt, size ); */
	}

	/* align */
	if( phys & 0xfff ) {
		size += (phys & 0xfff);
		virt -= (phys & 0xfff);
		phys &= ~0xfff;
	}
	if( size & 0xfff )
		size = (size + 0xfff) & ~0xfff;

	/* printk("...free memory found... phys: %08lX, virt: %08lX, size %lX\n", phys, virt, size ); */
	ofmem_map( phys, virt, size, -1 );
	return virt + offs;
}


/************************************************************************/
/* keep track of ea -> phys translations                                */
/************************************************************************/

static void split_trans( ucell virt )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	translation_t *t, *t2;

	for( t=ofmem->trans; t; t=t->next ) {
		if( virt > t->virt && virt < t->virt + t->size-1 ) {
			t2 = (translation_t*)malloc( sizeof(translation_t) );
			t2->virt = virt;
			t2->size = t->size - (virt - t->virt);
			t->size = virt - t->virt;
			t2->phys = t->phys + t->size;
			t2->mode = t->mode;
			t2->next = t->next;
			t->next = t2;
		}
	}
}

int ofmem_map_page_range( ucell phys, ucell virt, ucell size, ucell mode )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	translation_t *t, **tt;

	OFMEM_TRACE("ofmem_map_page_range " FMT_ucellx
			" -> " FMT_ucellx " " FMT_ucellx " mode " FMT_ucellx "\n",
			virt, phys, size, mode );

	split_trans( virt );
	split_trans( virt + size );

	/* detect remappings */
	for( t=ofmem->trans; t; ) {
		if( virt == t->virt || (virt < t->virt && virt + size > t->virt )) {
			if( t->phys + virt - t->virt != phys ) {
				OFMEM_TRACE("mapping altered virt=" FMT_ucellx ")\n", t->virt );
			} else if( t->mode != mode ){
				OFMEM_TRACE("mapping mode altered virt=" FMT_ucellx
						" old mode=" FMT_ucellx " new mode=" FMT_ucellx "\n",
						t->virt, t->mode, mode);
			}

			for( tt=&ofmem->trans; *tt != t ; tt=&(**tt).next ) {
			}

			*tt = t->next;

			/* really unmap these pages */
			ofmem_arch_unmap_pages(t->virt, t->size);

			free((char*)t);

			t=ofmem->trans;
			continue;
		}
		t=t->next;
	}

	/* add mapping */
	for( tt=&ofmem->trans; *tt && (**tt).virt < virt ; tt=&(**tt).next ) {
	}

	t = (translation_t*)malloc( sizeof(translation_t) );
	t->virt = virt;
	t->phys = phys;
	t->size = size;
	t->mode = mode;
	t->next = *tt;
	*tt = t;

	ofmem_update_translations();

	return 0;
}

static int unmap_page_range( ucell virt, ucell size )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	translation_t **plink;

	/* make sure there is exactly one matching translation entry */

	split_trans( virt );
	split_trans( virt + size );

	/* find and unlink entries in range */
	plink = &ofmem->trans;

	while (*plink && (*plink)->virt < virt+size) {
		translation_t **plinkentry = plink;
		translation_t *t = *plink;

		/* move ahead */
		plink = &t->next;

		if (t->virt >= virt && t->virt + t->size <= virt+size) {

			/* unlink entry */
			*plinkentry = t->next;

			OFMEM_TRACE("unmap_page_range found "
					FMT_ucellx " -> " FMT_ucellx " " FMT_ucellx
					" mode " FMT_ucellx "\n",
					t->virt, t->phys, t->size, t->mode );

			// really map these pages
			ofmem_arch_unmap_pages(t->virt, t->size);

			free((char*)t);
		}
	}

	ofmem_update_translations();

	return 0;
}

int ofmem_map( ucell phys, ucell virt, ucell size, ucell mode )
{
	/* printk("+ofmem_map: %08lX --> %08lX (size %08lX, mode 0x%02X)\n",
	   virt, phys, size, mode ); */

	if( (phys & 0xfff) || (virt & 0xfff) || (size & 0xfff) ) {

		OFMEM_TRACE("ofmem_map: Bad parameters ("
				FMT_ucellX " " FMT_ucellX " " FMT_ucellX ")\n",
				phys, virt, size );

		phys &= ~0xfff;
		virt &= ~0xfff;
		size = (size + 0xfff) & ~0xfff;
	}

#if defined(OFMEM_FILL_RANGE)
	{
		ofmem_t *ofmem = ofmem_arch_get_private();
		/* claim any unclaimed virtual memory in the range */
		fill_range( virt, size, &ofmem->virt_range );
		/* hmm... we better claim the physical range too */
		fill_range( phys, size, &ofmem->phys_range );
	}
#endif

	if (mode==-1) {
		mode = ofmem_arch_default_translation_mode(phys);
	}

	/* install translations */
	ofmem_map_page_range(phys, virt, size, mode);

	/* allow arch to install mappings early, e.g. for locked mappings */
	ofmem_arch_early_map_pages(phys, virt, size, mode);

	return 0;
}

int ofmem_unmap( ucell virt, ucell size )
{
	OFMEM_TRACE("ofmem_unmap " FMT_ucellx " " FMT_ucellx "\n",
			virt, size );

	if( (virt & 0xfff) || (size & 0xfff) ) {
		/* printk("ofmem_unmap: Bad parameters (%08lX %08lX)\n",
				virt, size ); */
		virt &= ~0xfff;
		size = (size + 0xfff) & ~0xfff;
	}

	/* remove translations and unmap pages */
	unmap_page_range(virt, size);

	return 0;
}

/* virtual -> physical. */
ucell ofmem_translate( ucell virt, ucell *mode )
{
	ofmem_t *ofmem = ofmem_arch_get_private();
	translation_t *t;

	for( t=ofmem->trans; t && t->virt <= virt ; t=t->next ) {
		ucell offs;
		if( t->virt + t->size - 1 < virt )
			continue;
		offs = virt - t->virt;
		*mode = t->mode;
		return t->phys + offs;
	}

	/*printk("ofmem_translate: no translation defined (%08lx)\n", virt);*/
	/*print_trans();*/
	return -1;
}

/* release memory allocated by ofmem_claim_phys */
void ofmem_release_phys( ucell phys, ucell size )
{
    OFMEM_TRACE("ofmem_release_phys addr=" FMT_ucellx " size=" FMT_ucellx "\n",
                phys, size);

	OFMEM_TRACE("ofmem_release_phys not implemented");
}

/* release memory allocated by ofmem_claim_virt */
void ofmem_release_virt( ucell virt, ucell size )
{
    OFMEM_TRACE("ofmem_release_virt addr=" FMT_ucellx " size=" FMT_ucellx "\n",
                virt, size);

	OFMEM_TRACE("ofmem_release_virt not implemented");
}

/************************************************************************/
/* init / cleanup                                                       */
/************************************************************************/

void ofmem_register( phandle_t ph_memory, phandle_t ph_mmu )
{
	s_phandle_memory = ph_memory;
	s_phandle_mmu = ph_mmu;

	ofmem_update_translations();
}
