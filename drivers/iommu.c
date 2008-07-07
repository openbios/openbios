/**
 ** Proll (PROM replacement)
 ** iommu.c: Functions for DVMA management.
 ** Copyright 1999 Pete Zaitcev
 ** This code is licensed under GNU General Public License.
 **/
#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/kernel.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"
#include "libc/string.h"

#include "openbios/drivers.h"
#include "asm/asi.h"
#include "asm/crs.h"
#include "asm/io.h"
#include "pgtsrmmu.h"
#include "iommu.h"

#define IOMMU_REGS  0x300
#define NCTX_SWIFT  0x100

#define IOPERM        (IOPTE_CACHE | IOPTE_WRITE | IOPTE_VALID)
#define MKIOPTE(phys) (((((phys)>>4) & IOPTE_PAGE) | IOPERM) & ~IOPTE_WAZ)
#define LOWMEMSZ 32 * 1024 * 1024

#ifdef CONFIG_DEBUG_IOMMU
#define DPRINTF(fmt, args...)                   \
    do { printk(fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...)
#endif

/*
 * Allocatable memory chunk.
 */
struct mem {
    char *start, *uplim;
    char *curp;
};

struct mem cmem;                /* Current memory, virtual */
struct mem cio;                 /* Current I/O space */

unsigned int va_shift;

static unsigned long *context_table;
static unsigned long *l1;

/*
 * IOMMU parameters
 */
struct iommu {
    struct iommu_regs *regs;
    unsigned int *page_table;
    unsigned long plow;     /* Base bus address */
    unsigned long vasize;   /* Size of VA region that we manage */
    struct mem bmap;
};

struct iommu ciommu;
static struct iommu_regs *regs;

static void iommu_init(struct iommu *t, uint64_t base);

static void
iommu_invalidate(struct iommu_regs *iregs)
{
    iregs->tlbflush = 0;
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
        return 0;
    t->curp = p + size;

    return p;
}

void *
mem_zalloc(struct mem *t, int size, int align)
{
    char *p;

    if ((p = mem_alloc(t, size, align)) != 0)
        memset(p, 0, size);

    return p;
}

static unsigned long
find_pte(unsigned long va, int alloc)
{
    uint32_t pte;
    void *p;
    unsigned long pa;

    pte = l1[(va >> SRMMU_PGDIR_SHIFT) & (SRMMU_PTRS_PER_PGD - 1)];
    if ((pte & SRMMU_ET_MASK) == SRMMU_ET_INVALID) {
        if (alloc) {
            p = mem_zalloc(&cmem, SRMMU_PTRS_PER_PMD * sizeof(int),
                           SRMMU_PTRS_PER_PMD * sizeof(int));
            if (p == 0)
                return -1;
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
            p = mem_zalloc(&cmem, SRMMU_PTRS_PER_PTE * sizeof(void *),
                           SRMMU_PTRS_PER_PTE * sizeof(void *));
            if (p == 0)
                return -2;
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
    if (type) {		/* I/O */
        pte |= SRMMU_REF;
        /* SRMMU cannot make Supervisor-only, but not exectutable */
        pte |= SRMMU_PRIV;
    } else {		/* memory */
        pte |= SRMMU_REF | SRMMU_CACHE;
        pte |= SRMMU_PRIV; /* Supervisor only access */
    }
    *(uint32_t *)pa = pte;
    DPRINTF("map_page: va 0x%lx pa 0x%llx pte 0x%x\n", va, epa, pte);

    return 0;
}

/*
 * Create an I/O mapping to pa[size].
 * Returns va of the mapping or 0 if unsuccessful.
 */
void *
map_io(uint64_t pa, int size)
{
    void *va;
    unsigned int npages;
    unsigned int off;
    unsigned int mva;

    off = pa & (PAGE_SIZE - 1);
    npages = (off + size - 1) / PAGE_SIZE + 1;
    pa &= ~(PAGE_SIZE - 1);

    va = mem_alloc(&cio, npages * PAGE_SIZE, PAGE_SIZE);
    if (va == 0)
        return va;

    mva = (unsigned int) va;
    DPRINTF("map_io: va 0x%p pa 0x%llx off 0x%x npages %d\n", va, pa, off, npages); /* P3 */
    while (npages-- != 0) {
        map_page(mva, pa, 1);
        mva += PAGE_SIZE;
        pa += PAGE_SIZE;
    }

    return (void *)((unsigned int)va + off);
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
map_pages(void)
{
    unsigned long va;
    int size;
    uint64_t pa;

    size = POP();
    va = POP();
    pa = POP();
    pa <<= 32;
    pa |= POP() & 0xffffffff;

    for (; size > 0; size -= PAGE_SIZE, pa += PAGE_SIZE, va += PAGE_SIZE)
        map_page(va, pa, 1);
    DPRINTF("map-page: va 0x%lx pa 0x%lx size 0x%x\n", va, pa, size);
}


void
ob_init_mmu(uint64_t base)
{
    extern unsigned int qemu_mem_size;

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

    PUSH(base >> 32);
    fword("encode-int");
    PUSH(base & 0xffffffff);
    fword("encode-int");
    fword("encode+");
    PUSH(IOMMU_REGS);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

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

    push_str("/iommu");
    fword("find-device");
    PUSH((unsigned long)regs);
    fword("encode-int");
    push_str("address");
    fword("property");

    PUSH(base >> 32);
    fword("encode-int");
    PUSH(base & 0xffffffff);
    fword("encode-int");
    fword("encode+");
    PUSH(IOMMU_REGS);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    PUSH(0);
    fword("active-package!");
    bind_func("pgmap@", pgmap_fetch);
    bind_func("pgmap!", pgmap_store);
    bind_func("map-pages", map_pages);
}


/*
 * Switch page tables.
 */
void
init_mmu_swift(uint64_t base)
{
    unsigned int addr, i;
    unsigned long pa, va;

    mem_init(&cio, (char *)&_end, (char *)&_iomem);

    context_table = mem_zalloc(&cmem, NCTX_SWIFT * sizeof(int), NCTX_SWIFT * sizeof(int));
    l1 = mem_zalloc(&cmem, 256 * sizeof(int), 256 * sizeof(int));

    context_table[0] = (((unsigned long)va2pa((unsigned long)l1)) >> 4) | SRMMU_ET_PTD;

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
    iommu_init(&ciommu, base);
}
/*
 * XXX This is a problematic interface. We alloc _memory_ which is uncached.
 * So if we ever reuse allocations somebody is going to get uncached pages.
 * Returned address is always aligned by page.
 * BTW, we were not going to give away anonymous storage, were we not?
 */
void *
dvma_alloc(int size, unsigned int *pphys)
{
    void *va;
    unsigned int pa, ba;
    unsigned int npages;
    unsigned int mva, mpa;
    unsigned int i;
    unsigned int *iopte;
    struct iommu *t = &ciommu;

    npages = (size + (PAGE_SIZE-1)) / PAGE_SIZE;
    va = mem_alloc(&cmem, npages * PAGE_SIZE, PAGE_SIZE);
    if (va == 0)
        return 0;

    ba = (unsigned int)mem_alloc(&t->bmap, npages * PAGE_SIZE, PAGE_SIZE);
    if (ba == 0)
        return 0;

    pa = (unsigned int)va2pa((unsigned long)va);

    /*
     * Change page attributes in MMU to uncached.
     */
    mva = (unsigned int) va;
    mpa = (unsigned int) pa;
    for (i = 0; i < npages; i++) {
        map_page(mva, mpa, 1);
        mva += PAGE_SIZE;
        mpa += PAGE_SIZE;
    }

    /*
     * Map into IOMMU page table.
     */
    mpa = (unsigned int) pa;
    iopte = &t->page_table[(ba - t->plow) / PAGE_SIZE];
    for (i = 0; i < npages; i++) {
        *iopte++ = MKIOPTE(mpa);
        mpa += PAGE_SIZE;
    }

    *pphys = ba;

    return va;
}

/*
 * Initialize IOMMU
 * This looks like initialization of CPU MMU but
 * the routine is higher in food chain.
 */
static void
iommu_init(struct iommu *t, uint64_t base)
{
    unsigned int *ptab;
    int ptsize;
    unsigned int impl, vers;
    unsigned int tmp;

    regs = map_io(base, IOMMU_REGS);
    if (regs == 0) {
        DPRINTF("Cannot map IOMMU\n");
        for (;;) { }
    }
    t->regs = regs;
    impl = (regs->control & IOMMU_CTRL_IMPL) >> 28;
    vers = (regs->control & IOMMU_CTRL_VERS) >> 24;

    tmp = regs->control;
    tmp &= ~(IOMMU_CTRL_RNGE);

    tmp |= (IOMMU_RNGE_32MB | IOMMU_CTRL_ENAB);
    t->plow = 0xfe000000;		/* End - 32 MB */
    t->vasize = 0x2000000;		/* 32 MB */

    regs->control = tmp;
    iommu_invalidate(regs);

    /* Allocate IOMMU page table */
    /* Thremendous alignment causes great waste... */
    ptsize = (t->vasize/PAGE_SIZE) *  sizeof(int);
    if ((ptab = mem_zalloc(&cmem, ptsize, ptsize)) == 0) {
        DPRINTF("Cannot allocate IOMMU table [0x%x]\n", ptsize);
        for (;;) { }
    }
    t->page_table = ptab;

    /* flush_cache_all(); */
    /** flush_tlb_all(); **/
    tmp = (unsigned int)va2pa((unsigned long)ptab);
    regs->base = tmp >> 4;
    iommu_invalidate(regs);

    DPRINTF("IOMMU: impl %d vers %d page table at 0x%p (pa 0x%x) of size %d bytes\n",
            impl, vers, t->page_table, tmp, ptsize);

    mem_init(&t->bmap, (char*)t->plow, (char *)0xfffff000);
}
