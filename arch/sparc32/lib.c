/* lib.c
 * tag: simple function library
 *
 * Copyright (C) 2003 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "libc/vsprintf.h"
#include "openbios/bindings.h"
#include "ofmem.h"
#include "asm/asi.h"
#include "pgtsrmmu.h"
#include "openprom.h"
#include "sys_info.h"
#include "boot.h"

#define NCTX_SWIFT  0x100
#define LOWMEMSZ 32 * 1024 * 1024

#ifdef CONFIG_DEBUG_MEM
#define DPRINTF(fmt, args...)                   \
    do { printk(fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...)
#endif

/* Format a string and print it on the screen, just like the libc
 * function printf.
 */
int printk( const char *fmt, ... )
{
        char *p, buf[512];
	va_list args;
	int i;

	va_start(args, fmt);
        i = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	for( p=buf; *p; p++ )
		putchar(*p);
	return i;
}

/*
 * Allocatable memory chunk.
 */
struct mem {
    char *start, *uplim;
    char *curp;
};

static struct mem cmem;         /* Current memory, virtual */
static struct mem cio;          /* Current I/O space */
struct mem cdvmem;              /* Current device virtual memory space */

unsigned int va_shift;
static unsigned long *context_table;
static unsigned long *l1;

static struct linux_mlist_v0 totphys[1];
static struct linux_mlist_v0 totmap[1];
static struct linux_mlist_v0 totavail[1];

struct linux_mlist_v0 *ptphys;
struct linux_mlist_v0 *ptmap;
struct linux_mlist_v0 *ptavail;

static struct {
	char 			*next_malloc;
        int                     left;
	alloc_desc_t		*mfree;			/* list of free malloc blocks */

	range_t			*phys_range;
	range_t			*virt_range;

	translation_t		*trans;			/* this is really a translation_t */
} ofmem;
#define ALLOC_BLOCK (64 * 1024)

// XXX should be posix_memalign
static int
posix_memalign2(void **memptr, size_t alignment, size_t size)
{
	alloc_desc_t *d, **pp;
	char *ret;

	if( !size )
                return -1;

        size = (size + (alignment - 1)) & ~(alignment - 1);
	size += sizeof(alloc_desc_t);

	/* look in the freelist */
	for( pp=&ofmem.mfree; *pp && (**pp).size < size; pp = &(**pp).next )
		;

	/* waste at most 4K by taking an entry from the freelist */
	if( *pp && (**pp).size < size + 0x1000 ) {
		ret = (char*)*pp + sizeof(alloc_desc_t);
		memset( ret, 0, (**pp).size - sizeof(alloc_desc_t) );
		*pp = (**pp).next;
                *memptr = ret;
                return 0;
	}

	if( !ofmem.next_malloc || ofmem.left < size) {
                unsigned long alloc_size = ALLOC_BLOCK;
                if (size > ALLOC_BLOCK)
                    alloc_size = size;
                // Recover possible leftover
                if ((size_t)ofmem.left > sizeof(alloc_desc_t) + 4) {
                    alloc_desc_t *d_leftover;

                    d_leftover = (alloc_desc_t*)ofmem.next_malloc;
                    d_leftover->size = ofmem.left - sizeof(alloc_desc_t);
                    free((void *)((unsigned long)d_leftover +
                                  sizeof(alloc_desc_t)));
                }

                ofmem.next_malloc = mem_alloc(&cmem, alloc_size, 8);
                ofmem.left = alloc_size;
        }

	if( ofmem.left < size) {
		printk("out of malloc memory (%x)!\n", size );
                return -1;
	}
	d = (alloc_desc_t*) ofmem.next_malloc;
	ofmem.next_malloc += size;
	ofmem.left -= size;

	d->next = NULL;
	d->size = size;

	ret = (char*)d + sizeof(alloc_desc_t);
	memset( ret, 0, size - sizeof(alloc_desc_t) );
        *memptr = ret;
        return 0;
}

void *malloc(int size)
{
    int ret;
    void *mem;

    ret = posix_memalign2(&mem, 8, size);
    if (ret != 0)
        return NULL;
    return mem;
}

void free(void *ptr)
{
	alloc_desc_t **pp, *d;

	/* it is legal to free NULL pointers (size zero allocations) */
	if( !ptr )
		return;

	d = (alloc_desc_t*)((unsigned long)ptr - sizeof(alloc_desc_t));
	d->next = ofmem.mfree;

	/* insert in the (sorted) freelist */
	for( pp=&ofmem.mfree; *pp && (**pp).size < d->size ; pp = &(**pp).next )
		;
	d->next = *pp;
	*pp = d;
}

void *
realloc( void *ptr, size_t size )
{
	alloc_desc_t *d = (alloc_desc_t*)((unsigned long)ptr - sizeof(alloc_desc_t));
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

// XXX should be removed
int
posix_memalign(void **memptr, size_t alignment, size_t size)
{
    void *block;

    block = mem_alloc(&cmem, size, alignment);

    if (!block)
        return -1;

    *memptr = block;
    return 0;
}

/*
 * Allocate memory. This is reusable.
 */
void
mem_init(struct mem *t, char *begin, char *limit)
{
    t->start = begin;
    t->uplim = limit;
    t->curp = begin;
}

void *
mem_alloc(struct mem *t, int size, int align)
{
    char *p;
    unsigned long pa;

    // The alignment restrictions refer to physical, not virtual
    // addresses
    pa = va2pa((unsigned long)t->curp) + (align - 1);
    pa &= ~(align - 1);
    p = (char *)pa2va(pa);

    if ((unsigned long)p >= (unsigned long)t->uplim ||
        (unsigned long)p + size > (unsigned long)t->uplim)
        return NULL;
    t->curp = p + size;

    return p;
}

static unsigned long
find_pte(unsigned long va, int alloc)
{
    uint32_t pte;
    void *p;
    unsigned long pa;
    int ret;

    pte = l1[(va >> SRMMU_PGDIR_SHIFT) & (SRMMU_PTRS_PER_PGD - 1)];
    if ((pte & SRMMU_ET_MASK) == SRMMU_ET_INVALID) {
        if (alloc) {
            ret = posix_memalign(&p, SRMMU_PTRS_PER_PMD * sizeof(int),
                                 SRMMU_PTRS_PER_PMD * sizeof(int));
            if (ret != 0)
                return ret;
            pte = SRMMU_ET_PTD | ((va2pa((unsigned long)p)) >> 4);
            l1[(va >> SRMMU_PGDIR_SHIFT) & (SRMMU_PTRS_PER_PGD - 1)] = pte;
            /* barrier() */
        } else {
            return -1;
        }
    }

    pa = (pte & 0xFFFFFFF0) << 4;
    pa += ((va >> SRMMU_PMD_SHIFT) & (SRMMU_PTRS_PER_PMD - 1)) << 2;
    pte = *(uint32_t *)pa2va(pa);
    if ((pte & SRMMU_ET_MASK) == SRMMU_ET_INVALID) {
        if (alloc) {
            ret = posix_memalign(&p, SRMMU_PTRS_PER_PTE * sizeof(void *),
                                 SRMMU_PTRS_PER_PTE * sizeof(void *));
            if (ret != 0)
                return ret;
            pte = SRMMU_ET_PTD | ((va2pa((unsigned int)p)) >> 4);
            *(uint32_t *)pa2va(pa) = pte;
        } else {
            return -2;
        }
    }

    pa = (pte & 0xFFFFFFF0) << 4;
    pa += ((va >> PAGE_SHIFT) & (SRMMU_PTRS_PER_PTE - 1)) << 2;

    return pa2va(pa);
}

/*
 * Create a memory mapping from va to epa.
 */
int
map_page(unsigned long va, uint64_t epa, int type)
{
    uint32_t pte;
    unsigned long pa;

    pa = find_pte(va, 1);

    pte = SRMMU_ET_PTE | ((epa & PAGE_MASK) >> 4);
    if (type) {         /* I/O */
        pte |= SRMMU_REF;
        /* SRMMU cannot make Supervisor-only, but not exectutable */
        pte |= SRMMU_PRIV;
    } else {            /* memory */
        pte |= SRMMU_REF | SRMMU_CACHE;
        pte |= SRMMU_PRIV; /* Supervisor only access */
    }
    *(uint32_t *)pa = pte;
    DPRINTF("map_page: va 0x%lx pa 0x%llx pte 0x%x\n", va, epa, pte);

    return 0;
}

static void map_pages(unsigned long va, uint64_t pa, int type,
                      unsigned long size)
{
    unsigned long npages, off;

    DPRINTF("map_pages: va 0x%lx, pa 0x%llx, size 0x%lx\n", va, pa, size);

    off = pa & (PAGE_SIZE - 1);
    npages = (off + (size - 1) + (PAGE_SIZE - 1)) / PAGE_SIZE;
    pa &= ~(uint64_t)(PAGE_SIZE - 1);

    while (npages-- != 0) {
        map_page(va, pa, type);
        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
}
/*
 * Create an I/O mapping to pa[size].
 * Returns va of the mapping or 0 if unsuccessful.
 */
void *
map_io(uint64_t pa, int size)
{
    unsigned long va;
    unsigned int npages;
    unsigned int off;

    off = pa & (PAGE_SIZE - 1);
    npages = (off + size - 1) / PAGE_SIZE + 1;
    pa &= ~(PAGE_SIZE - 1);

    va = (unsigned long)mem_alloc(&cio, npages * PAGE_SIZE, PAGE_SIZE);
    if (va == 0)
        return NULL;

    map_pages(va, pa, 1, npages * PAGE_SIZE);
    return (void *)(va + off);
}

/*
 * D5.3 pgmap@ ( va -- pte )
 */
static void
pgmap_fetch(void)
{
    uint32_t pte;
    unsigned long va, pa;

    va = POP();

    pa = find_pte(va, 0);
    if (pa == 1 || pa == 2)
        goto error;
    pte = *(uint32_t *)pa;
    DPRINTF("pgmap@: va 0x%lx pa 0x%lx pte 0x%x\n", va, pa, pte);

    PUSH(pte);
    return;
 error:
    PUSH(0);
}

/*
 * D5.3 pgmap! ( pte va -- )
 */
static void
pgmap_store(void)
{
    uint32_t pte;
    unsigned long va, pa;

    va = POP();
    pte = POP();

    pa = find_pte(va, 1);
    *(uint32_t *)pa = pte;
    DPRINTF("pgmap!: va 0x%lx pa 0x%lx pte 0x%x\n", va, pa, pte);
}

/*
 * D5.3 map-pages ( pa space va size -- )
 */
static void
ob_map_pages(void)
{
    unsigned long va;
    int size;
    uint64_t pa;

    size = POP();
    va = POP();
    pa = POP();
    pa <<= 32;
    pa |= POP() & 0xffffffff;

    map_pages(va, pa, 0, size);
    DPRINTF("map-page: va 0x%lx pa 0x%llx size 0x%x\n", va, pa, size);
}

static void
init_romvec_mem(void)
{
    ptphys = totphys;
    ptmap = totmap;
    ptavail = totavail;

    /*
     * Form memory descriptors.
     */
    totphys[0].theres_more = NULL;
    totphys[0].start_adr = (char *) 0;
    totphys[0].num_bytes = qemu_mem_size;

    totavail[0].theres_more = NULL;
    totavail[0].start_adr = (char *) 0;
    totavail[0].num_bytes = va2pa((int)&_start) - PAGE_SIZE;

    totmap[0].theres_more = NULL;
    totmap[0].start_adr = &_start;
    totmap[0].num_bytes = (unsigned long) &_iomem -
        (unsigned long) &_start + PAGE_SIZE;
}

char *obp_dumb_mmap(char *va, int which_io, unsigned int pa,
                    unsigned int size)
{
    uint64_t mpa = ((uint64_t)which_io << 32) | (uint64_t)pa;

    map_pages((unsigned long)va, mpa, 0, size);
    return va;
}

void obp_dumb_munmap(__attribute__((unused)) char *va,
                     __attribute__((unused)) unsigned int size)
{
    DPRINTF("obp_dumb_munmap: virta 0x%x, sz %d\n", (unsigned int)va, size);
}

char *obp_dumb_memalloc(char *va, unsigned int size)
{
    static unsigned int next_free_address = 0xFFEDA000;

    size = (size + 7) & ~7;
    // XXX should use normal memory alloc
    totmap[0].num_bytes -= size;
    DPRINTF("obp_dumb_memalloc va 0x%p size %x at 0x%x\n", va, size,
            totmap[0].num_bytes);

    // If va is null, the allocator is supposed to pick a "suitable" address.
    // (See OpenSolaric prom_alloc.c)  There's not any real guidance as
    // to what might be "suitable".  So we mimic the behavior of a Sun boot
    // ROM.

    if (va == NULL) {
        // XXX should register virtual memory allocation
        va = (char *)(next_free_address - size);
        next_free_address -= size;
        DPRINTF("obp_dumb_memalloc req null -> 0x%p\n", va);
    }

    map_pages((unsigned long)va, totmap[0].num_bytes, 0, size);

    return va;
}

void obp_dumb_memfree(__attribute__((unused))char *va,
                             __attribute__((unused))unsigned sz)
{
    DPRINTF("obp_dumb_memfree 0x%p (size %d)\n", va, sz);
}

void
ob_init_mmu(void)
{
    push_str("/memory");
    fword("find-device");

    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(qemu_mem_size);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(va2pa((unsigned long)&_start) - PAGE_SIZE);
    fword("encode-int");
    fword("encode+");
    push_str("available");
    fword("property");

    push_str("/virtual-memory");
    fword("find-device");

    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_start - PAGE_SIZE);
    fword("encode-int");
    fword("encode+");

    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(va2pa((unsigned long)&_iomem));
    fword("encode-int");
    fword("encode+");
    PUSH(-va2pa((unsigned long)&_iomem));
    fword("encode-int");
    fword("encode+");
    push_str("available");
    fword("property");

    PUSH(0);
    fword("active-package!");
    bind_func("pgmap@", pgmap_fetch);
    bind_func("pgmap!", pgmap_store);
    bind_func("map-pages", ob_map_pages);

    init_romvec_mem();
}

/*
 * Switch page tables.
 */
void
init_mmu_swift(void)
{
    unsigned int addr, i;
    unsigned long pa, va;

    mem_init(&cmem, (char *) &_vmem, (char *)&_evmem);
    mem_init(&cio, (char *)&_end, (char *)&_iomem);

    posix_memalign((void *)&context_table, NCTX_SWIFT * sizeof(int),
                   NCTX_SWIFT * sizeof(int));
    posix_memalign((void *)&l1, 256 * sizeof(int), 256 * sizeof(int));

    context_table[0] = (((unsigned long)va2pa((unsigned long)l1)) >> 4) |
        SRMMU_ET_PTD;

    for (i = 1; i < NCTX_SWIFT; i++) {
        context_table[i] = SRMMU_ET_INVALID;
    }
    for (i = 0; i < 256; i += 4) {
        l1[i] = SRMMU_ET_INVALID;
    }

    // text, rodata, data, and bss mapped to end of RAM
    va = (unsigned long)&_start;
    for (; va < (unsigned long)&_end; va += PAGE_SIZE) {
        pa = va2pa(va);
        map_page(va, pa, 0);
    }

    // 1:1 mapping for RAM
    pa = va = 0;
    for (; va < LOWMEMSZ; va += PAGE_SIZE, pa += PAGE_SIZE) {
        map_page(va, pa, 0);
    }

    /*
     * Flush cache
     */
    for (addr = 0; addr < 0x2000; addr += 0x10) {
        __asm__ __volatile__ ("sta %%g0, [%0] %1\n\t" : :
                              "r" (addr), "i" (ASI_M_DATAC_TAG));
        __asm__ __volatile__ ("sta %%g0, [%0] %1\n\t" : :
                              "r" (addr<<1), "i" (ASI_M_TXTC_TAG));
    }
    srmmu_set_context(0);
    srmmu_set_ctable_ptr(va2pa((unsigned long)context_table));
    srmmu_flush_whole_tlb();
}
