/*
 *   Creation Date: <1999/11/07 19:02:11 samuel>
 *   Time-stamp: <2004/01/07 19:42:36 samuel>
 *
 *	<ofmem.c>
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
#include "ofmem.h"
#include "kernel.h"
#include "mmutypes.h"
#include "asm/processor.h"

#define BIT(n)		(1U<<(31-(n)))

/* called from assembly */
extern void dsi_exception( void );
extern void isi_exception( void );
extern void setup_mmu( ulong code_base );

#define FREE_BASE		0x00004000
#define IO_BASE			0x80000000
#define OFMEM			((ofmem_t*)FREE_BASE)
#define OF_MALLOC_BASE		((char*)OFMEM + ((sizeof(ofmem_t) + 3) & ~3))

#define HASH_SIZE		(2 << 15)

#define	SEGR_USER		BIT(2)
#define SEGR_BASE		0x0400

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

typedef struct {
	ulong			ramsize;
	char 			*next_malloc;
	alloc_desc_t		*mfree;		/* list of free malloc blocks */

	range_t			*phys_range;
	range_t			*virt_range;

	translation_t		*trans;	/* this is really a translation_t */
} ofmem_t;

static inline ulong
get_hash_base( void )
{
	ulong sdr1;

	asm volatile("mfsdr1 %0" : "=r" (sdr1) );

	return (sdr1 & 0xffff0000);
}

static inline ulong
get_hash_size( void )
{
	ulong sdr1;

	asm volatile("mfsdr1 %0" : "=r" (sdr1) );

	return ((sdr1 << 16) | 0x0000ffff) + 1;
}

ulong
get_ram_size( void )
{
	ofmem_t *ofmem = OFMEM;
	return ofmem->ramsize;
}

static inline ulong
get_rom_base( void )
{
	return get_ram_size() - 0x00100000;
}

ulong
get_ram_top( void )
{
	return get_rom_base() - HASH_SIZE - (32 + 32 + 64) * 1024;
}

ulong
get_ram_bottom( void )
{
        return (ulong)OF_MALLOC_BASE;
}

static phandle_t cpu_handle = 0;
static void
ofmem_update_translations( void )
{
	ofmem_t *ofmem = OFMEM;
	translation_t *t;
	int ncells;
	cell *props;

	if (cpu_handle == 0)
		return;

	for( t = ofmem->trans, ncells = 0; t ; t=t->next, ncells++ )
		;

	props = malloc(ncells * sizeof(cell) * 4);
	if (props == NULL)
		return;

	for( t = ofmem->trans, ncells = 0 ; t ; t=t->next ) {
		props[ncells++] = t->virt;
		props[ncells++] = t->size;
		props[ncells++] = t->phys;
		props[ncells++] = t->mode;
	}
	set_property(cpu_handle, "translations",
		     (char*)props, ncells * sizeof(cell));
	free(props);
}

/************************************************************************/
/*	OF private allocations						*/
/************************************************************************/

void *
malloc( int size )
{
	ofmem_t *ofmem = OFMEM;
	alloc_desc_t *d, **pp;
	char *ret;
	ulong top;

	if( !size )
		return NULL;

	if( !ofmem->next_malloc )
		ofmem->next_malloc = (char*)OF_MALLOC_BASE;

	if( size & 3 )
		size += 4 - (size & 3);
	size += sizeof(alloc_desc_t);

	/* look in the freelist */
	for( pp=&ofmem->mfree; *pp && (**pp).size < size; pp = &(**pp).next )
		;

	/* waste at most 4K by taking an entry from the freelist */
	if( *pp && (**pp).size < size + 0x1000 ) {
		ret = (char*)*pp + sizeof(alloc_desc_t);
		memset( ret, 0, (**pp).size - sizeof(alloc_desc_t) );
		*pp = (**pp).next;
		return ret;
	}

	top = get_hash_base() - (32 + 64) * 1024;
	if( (ulong)ofmem->next_malloc + size > top ) {
		printk("out of malloc memory (%x)!\n", size );
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

void
free( void *ptr )
{
	ofmem_t *ofmem = OFMEM;
	alloc_desc_t **pp, *d;

	/* it is legal to free NULL pointers (size zero allocations) */
	if( !ptr )
		return;

        d = (alloc_desc_t*)((char *)ptr - sizeof(alloc_desc_t));
	d->next = ofmem->mfree;

	/* insert in the (sorted) freelist */
	for( pp=&ofmem->mfree; *pp && (**pp).size < d->size ; pp = &(**pp).next )
		;
	d->next = *pp;
	*pp = d;
}

void *
realloc( void *ptr, size_t size )
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
/*	misc								*/
/************************************************************************/

extern char _start[], _end[];
static inline ucell
def_memmode( ucell phys )
{
	/* XXX: Guard bit not set as it should! */
	if( phys < IO_BASE )
		return 0x02;	/*0xa*/	/* wim GxPp */
	return 0x6a;		/* WIm GxPp, I/O */
}


/************************************************************************/
/*	client interface						*/
/************************************************************************/

static int
is_free( ucell ea, ucell size, range_t *r )
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

static void
add_entry_( ucell ea, ucell size, range_t **r )
{
	range_t *nr;

	for( ; *r && (**r).start < ea; r=&(**r).next )
		;
	nr = (range_t*)malloc( sizeof(range_t) );
	nr->next = *r;
	nr->start = ea;
	nr->size = size;
	*r = nr;
}

static int
add_entry( ucell ea, ucell size, range_t **r )
{
	if( !is_free( ea, size, *r ) ) {
		printk("add_entry: range not free!\n");
		return -1;
	}
	add_entry_( ea, size, r );
	return 0;
}

static void
join_ranges( range_t **rr )
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

static void
fill_range( ulong ea, ucell size, range_t **rr )
{
	add_entry_( ea, size, rr );
	join_ranges( rr );
}

static ucell
find_area( ucell align, ucell size, range_t *r, ucell min, ucell max, int reverse )
{
	ucell base = min;
	range_t *r2;

	if( (align & (align-1)) ) {
		printk("bad alignment %d\n", align);
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
			for( rp=r; rp && rp->next != r2 ; rp=rp->next )
				;
			r2 = rp;
			if( !r2 )
				break;
			base = r2->start - size;
		}
	}
	return (ucell)-1;
}

static ucell
ofmem_claim_phys_( ucell phys, ucell size, ucell align, ucell min, ucell max, int reverse )
{
	ofmem_t *ofmem = OFMEM;
	if( !align ) {
		if( !is_free( phys, size, ofmem->phys_range ) ) {
			printk("Non-free physical memory claimed!\n");
			return -1;
		}
		add_entry( phys, size, &ofmem->phys_range );
		return phys;
	}
	phys = find_area( align, size, ofmem->phys_range, min, max, reverse );
	if( phys == (ulong)-1 ) {
		printk("ofmem->claim_phys - out of space\n");
		return -1;
	}
	add_entry( phys, size, &ofmem->phys_range );
	return phys;
}

/* if align != 0, phys is ignored. Returns -1 on error */
ucell
ofmem_claim_phys( ucell phys, ucell size, ucell align )
{
	/* printk("+ ofmem_claim phys %08lx %lx %ld\n", phys, size, align ); */
	return ofmem_claim_phys_( phys, size, align, 0, get_ram_size(), 0 );
}

static ucell
ofmem_claim_virt_( ucell virt, ucell size, ucell align, ucell min, ucell max, int reverse )
{
	ofmem_t *ofmem = OFMEM;
	if( !align ) {
		if( !is_free( virt, size, ofmem->virt_range ) ) {
			printk("Non-free physical memory claimed!\n");
			return -1;
		}
		add_entry( virt, size, &ofmem->virt_range );
		return virt;
	}

	virt = find_area( align, size, ofmem->virt_range, min, max, reverse );
	if( virt == (ulong)-1 ) {
		printk("ofmem_claim_virt - out of space\n");
		return -1;
	}
	add_entry( virt, size, &ofmem->virt_range );
	return virt;
}

ucell
ofmem_claim_virt( ucell virt, ucell size, ucell align )
{
	/* printk("+ ofmem_claim virt %08lx %lx %ld\n", virt, size, align ); */
	return ofmem_claim_virt_( virt, size, align, get_ram_size() , IO_BASE, 0 );
}


/* allocate both physical and virtual space and add a translation */
ucell
ofmem_claim( ucell addr, ucell size, ucell align )
{
	ofmem_t *ofmem = OFMEM;
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
			/* printk("**** ofmem_claim failure ***!\n"); */
			return -1;
		}
	} else {
		if( align < 0x1000 )
			align = 0x1000;
		phys = ofmem_claim_phys_( addr, size, align, 0, get_ram_size(), 1 /* reverse */ );
		virt = ofmem_claim_virt_( addr, size, align, 0, get_ram_size(), 1 /* reverse */ );
		if( phys == (ulong)-1 || virt == (ulong)-1 ) {
			printk("ofmem_claim failed\n");
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
	ofmem_map( phys, virt, size, def_memmode(phys) );
	return virt + offs;
}


/************************************************************************/
/*	keep track of ea -> phys translations				*/
/************************************************************************/

static void
split_trans( ulong virt )
{
	ofmem_t *ofmem = OFMEM;
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

static int
map_page_range( ucell virt, ucell phys, ucell size, ucell mode )
{
	ofmem_t *ofmem = OFMEM;
	translation_t *t, **tt;

	split_trans( virt );
	split_trans( virt + size );

	/* detect remappings */
	for( t=ofmem->trans; t; ) {
		if( virt == t->virt || (virt < t->virt && virt + size > t->virt )) {
			if( t->phys + virt - t->virt != phys ) {
				printk("mapping altered (ea %08x)\n", t->virt );
			} else if( t->mode != mode ){
				printk("mapping mode altered\n");
			}
			for( tt=&ofmem->trans; *tt != t ; tt=&(**tt).next )
				;
			*tt = t->next;
			free((char*)t);
			t=ofmem->trans;
			continue;
		}
		t=t->next;
	}
	/* add mapping */
	for( tt=&ofmem->trans; *tt && (**tt).virt < virt ; tt=&(**tt).next )
		;
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

int
ofmem_map( ucell phys, ucell virt, ucell size, ucell mode )
{
	ofmem_t *ofmem = OFMEM;
	/* printk("+ofmem_map: %08lX --> %08lX (size %08lX, mode 0x%02X)\n",
	   virt, phys, size, mode ); */

	if( (phys & 0xfff) || (virt & 0xfff) || (size & 0xfff) ) {
		/* printk("ofmem_map: Bad parameters (%08lX %08lX %08lX)\n",
		       phys, virt, size ); */
		phys &= ~0xfff;
		virt &= ~0xfff;
		size = (size + 0xfff) & ~0xfff;
	}
#if 1
	/* claim any unclaimed virtual memory in the range */
	fill_range( virt, size, &ofmem->virt_range );
	/* hmm... we better claim the physical range too */
	fill_range( phys, size, &ofmem->phys_range );
#endif
	//printk("map_page_range %08lx -> %08lx %08lx\n", virt, phys, size );
	map_page_range( virt, phys, size, (mode==-1)? def_memmode(phys) : mode );
	return 0;
}

/* virtual -> physical. */
ucell
ofmem_translate( ucell virt, ucell *mode )
{
	ofmem_t *ofmem = OFMEM;
	translation_t *t;

	for( t=ofmem->trans; t && t->virt <= virt ; t=t->next ) {
		ucell offs;
		if( t->virt + t->size - 1 < virt )
			continue;
		offs = virt - t->virt;
		*mode = t->mode;
		return t->phys + offs;
	}

	//printk("ofmem_translate: no translation defined (%08lx)\n", virt);
	//print_trans();
	return -1UL;
}

/* release memory allocated by ofmem_claim */
void
ofmem_release( ucell virt, ucell size )
{
	/* printk("ofmem_release unimplemented (%08lx, %08lx)\n", virt, size ); */
}


/************************************************************************/
/*	page fault handler						*/
/************************************************************************/

static ucell
ea_to_phys( ucell ea, ucell *mode )
{
	ucell phys;

	if (ea >= 0xfff00000UL) {
		/* ROM into RAM */
		ea -= 0xfff00000UL;
		phys = get_rom_base() + ea;
		*mode = 0x02;
		return phys;
	}

	phys = ofmem_translate(ea, mode);
	if( phys == -1UL ) {
		phys = ea;
		*mode = def_memmode( phys );

		/* print_virt_range(); */
		/* print_phys_range(); */
		/* print_trans(); */
	}
	return phys;
}

static void
hash_page_64( ucell ea, ucell phys, ucell mode )
{
	static int next_grab_slot=0;
	uint64_t vsid_mask, page_mask, pgidx, hash;
	uint64_t htab_mask, mask, avpn;
	ulong pgaddr;
	int i, found;
	unsigned int vsid, vsid_sh, sdr, sdr_sh, sdr_mask;
	mPTE_64_t *pp;

	vsid = (ea >> 28) + SEGR_BASE;
	vsid_sh = 7;
	vsid_mask = 0x00003FFFFFFFFF80ULL;
	asm ( "mfsdr1 %0" : "=r" (sdr) );
	sdr_sh = 18;
	sdr_mask = 0x3FF80;
	page_mask = 0x0FFFFFFF; // XXX correct?
	pgidx = (ea & page_mask) >> PAGE_SHIFT;
	avpn = (vsid << 12) | ((pgidx >> 4) & 0x0F80);;

	hash = ((vsid ^ pgidx) << vsid_sh) & vsid_mask;
	htab_mask = 0x0FFFFFFF >> (28 - (sdr & 0x1F));
	mask = (htab_mask << sdr_sh) | sdr_mask;
	pgaddr = sdr | (hash & mask);
	pp = (mPTE_64_t *)pgaddr;

	/* replace old translation */
	for( found=0, i=0; !found && i<8; i++ )
		if( pp[i].avpn == avpn )
			found=1;

	/* otherwise use a free slot */
	for( i=0; !found && i<8; i++ )
		if( !pp[i].v )
			found=1;

	/* out of slots, just evict one */
	if( !found ) {
		i = next_grab_slot + 1;
		next_grab_slot = (next_grab_slot + 1) % 8;
	}
	i--;
	{
	mPTE_64_t p = {
		// .avpn_low = avpn,
		.avpn = avpn >> 7,
		.h = 0,
		.v = 1,

		.rpn = (phys & ~0xfff) >> 12,
		.r = mode & (1 << 8) ? 1 : 0,
		.c = mode & (1 << 7) ? 1 : 0,
		.w = mode & (1 << 6) ? 1 : 0,
		.i = mode & (1 << 5) ? 1 : 0,
		.m = mode & (1 << 4) ? 1 : 0,
		.g = mode & (1 << 3) ? 1 : 0,
		.n = mode & (1 << 2) ? 1 : 0,
		.pp = mode & 3,
	};
	pp[i] = p;
	}

	asm volatile( "tlbie %0"  :: "r"(ea) );
}

static void
hash_page_32( ucell ea, ucell phys, ucell mode )
{
	static int next_grab_slot=0;
	ulong *upte, cmp, hash1;
	int i, vsid, found;
	mPTE_t *pp;

	vsid = (ea>>28) + SEGR_BASE;
	cmp = BIT(0) | (vsid << 7) | ((ea & 0x0fffffff) >> 22);

	hash1 = vsid;
	hash1 ^= (ea >> 12) & 0xffff;
	hash1 &= (get_hash_size() - 1) >> 6;

	pp = (mPTE_t*)(get_hash_base() + (hash1 << 6));
	upte = (ulong*)pp;

	/* replace old translation */
	for( found=0, i=0; !found && i<8; i++ )
		if( cmp == upte[i*2] )
			found=1;

	/* otherwise use a free slot */
	for( i=0; !found && i<8; i++ )
		if( !pp[i].v )
			found=1;

	/* out of slots, just evict one */
	if( !found ) {
		i = next_grab_slot + 1;
		next_grab_slot = (next_grab_slot + 1) % 8;
	}
	i--;
	upte[i*2] = cmp;
	upte[i*2+1] = (phys & ~0xfff) | mode;

	asm volatile( "tlbie %0"  :: "r"(ea) );
}

static int is_ppc64(void)
{
	unsigned int pvr;
	asm volatile("mfspr %0, 0x11f" : "=r" (pvr) );

	return ((pvr >= 0x330000) && (pvr < 0x70330000));
}

static void hash_page( ulong ea, ulong phys, ucell mode )
{
	if ( is_ppc64() )
		hash_page_64(ea, phys, mode);
	else
		hash_page_32(ea, phys, mode);
}

void
dsi_exception( void )
{
	ulong dar, dsisr;
	ucell mode;
	ucell phys;

	asm volatile("mfdar %0" : "=r" (dar) : );
	asm volatile("mfdsisr %0" : "=r" (dsisr) : );

	phys = ea_to_phys(dar, &mode);
	hash_page( dar, phys, mode );
}

void
isi_exception( void )
{
	ulong nip, srr1;
	ucell mode;
	ucell phys;

	asm volatile("mfsrr0 %0" : "=r" (nip) : );
	asm volatile("mfsrr1 %0" : "=r" (srr1) : );

	phys = ea_to_phys(nip, &mode);
	hash_page( nip, phys, mode );
}


/************************************************************************/
/*	init / cleanup							*/
/************************************************************************/

void
setup_mmu( ulong ramsize )
{
	ofmem_t *ofmem = OFMEM;
	ulong sdr1, sr_base, msr;
	ulong hash_base;
	ulong hash_mask = 0xffff0000;
	int i;

	memset(ofmem, 0, sizeof(ofmem_t));
	ofmem->ramsize = ramsize;

	/* SDR1: Storage Description Register 1 */

	if(is_ppc64())
		hash_mask = 0xfff00000;

	hash_base = (ramsize - 0x00100000 - HASH_SIZE) & hash_mask;
        memset((void *)hash_base, 0, HASH_SIZE);
	sdr1 = hash_base | ((HASH_SIZE-1) >> 16);
	asm volatile("mtsdr1 %0" :: "r" (sdr1) );

	/* Segment Register */

	sr_base = SEGR_USER | SEGR_BASE ;
	for( i=0; i<16; i++ ) {
		int j = i << 28;
		asm volatile("mtsrin %0,%1" :: "r" (sr_base + i), "r" (j) );
	}

	memcpy((void *)get_rom_base(), (void *)0xfff00000, 0x00100000);

	/* Enable MMU */

	asm volatile("mfmsr %0" : "=r" (msr) : );
	msr |= MSR_IR | MSR_DR;
	asm volatile("mtmsr %0" :: "r" (msr) );
}

void
ofmem_init( void )
{
	ofmem_claim_phys( 0, get_ram_bottom(), 0 );
	ofmem_claim_virt( 0, get_ram_bottom(), 0 );
	ofmem_claim_phys( get_ram_top(), get_ram_size() - get_ram_top(), 0);
	ofmem_claim_virt( get_ram_top(), get_ram_size() - get_ram_top(), 0);
}

void
ofmem_register( phandle_t ph )
{
	cpu_handle = ph;
	ofmem_update_translations();
}
