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

#define PHYS_JJ_IOMMU 0x10000000 /* First page of sun4m IOMMU */
#define IOPERM        (IOPTE_CACHE | IOPTE_WRITE | IOPTE_VALID)
#define MKIOPTE(phys) (((((phys)>>4) & IOPTE_PAGE) | IOPERM) & ~IOPTE_WAZ)
#define LOWMEMSZ 32 * 1024 * 1024
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

#define NCTX_SWIFT  0x100

static void iommu_init(struct iommu *t);
static void
iommu_invalidate(struct iommu_regs *regs) {
    regs->tlbflush = 0;
}

/*
 * Allocate memory. This is reusable.
 */
static void
mem_init(struct mem *t, char *begin, char *limit)
{
    t->start = begin;
    t->uplim = limit;
    t->curp = begin;
}

static void
mem_fini(struct mem *t)
{
    t->curp = 0;
}

void *
mem_alloc(struct mem *t, int size, int align)
{
    char *p;

    p = (char *)((((unsigned int)t->curp) + (align-1)) & ~(align-1));
    if (p >= t->uplim || p + size > t->uplim)
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

/*
 * Create a memory mapping from va to epa in page table pgd.
 * highbase is used for v2p translation.
 */
int
map_page(unsigned long va, unsigned long epa, int type)
{
    uint32_t pte;
    void *p;
    unsigned long pa;

    pte = l1[(va >> SRMMU_PGDIR_SHIFT) & (SRMMU_PTRS_PER_PGD - 1)];
    if ((pte & SRMMU_ET_MASK) == SRMMU_ET_INVALID) {
        p = mem_zalloc(&cmem, SRMMU_PTRS_PER_PMD * sizeof(int),
                       SRMMU_PTRS_PER_PMD * sizeof(int));
        if (p == 0)
            goto drop;
        pte = SRMMU_ET_PTD | ((va2pa((unsigned long)p)) >> 4);
        l1[(va >> SRMMU_PGDIR_SHIFT) & (SRMMU_PTRS_PER_PGD - 1)] = pte;
        /* barrier() */
    }

    pa = (pte & 0xFFFFFFF0) << 4;
    pa += ((va >> SRMMU_PMD_SHIFT) & (SRMMU_PTRS_PER_PMD - 1)) << 2;
    pte = *(uint32_t *)pa2va(pa);
    if ((pte & SRMMU_ET_MASK) == SRMMU_ET_INVALID) {
        p = mem_zalloc(&cmem, SRMMU_PTRS_PER_PTE * sizeof(void *),
                       SRMMU_PTRS_PER_PTE * sizeof(void *));
        if (p == 0)
            goto drop;
        pte = SRMMU_ET_PTD | ((va2pa((unsigned int)p)) >> 4);
        *(uint32_t *)pa2va(pa) = pte;
    }

    pa = (pte & 0xFFFFFFF0) << 4;
    pa += ((va >> PAGE_SHIFT) & (SRMMU_PTRS_PER_PTE - 1)) << 2;

    pte = SRMMU_ET_PTE | ((epa & PAGE_MASK) >> 4);
    if (type) {		/* I/O */
        pte |= SRMMU_REF;
        /* SRMMU cannot make Supervisor-only, but not exectutable */
        pte |= SRMMU_PRIV;
    } else {		/* memory */
        pte |= SRMMU_REF | SRMMU_CACHE;
        pte |= SRMMU_PRIV; /* Supervisor only access */
    }
    *(uint32_t *)pa2va(pa) = pte;
    //printk("map_page: va 0x%lx pa 0x%lx pte 0x%x\n", va, epa, pte);

    return 0;

 drop:

    return -1;
}

/*
 * Create an I/O mapping to pa[size].
 * Returns va of the mapping or 0 if unsuccessful.
 */
void *
map_io(unsigned pa, int size)
{
    void *va;
    unsigned int npages;
    unsigned int off;
    unsigned int mva;

    off = pa & (PAGE_SIZE - 1);
    npages = (off + size + (PAGE_SIZE - 1)) / PAGE_SIZE;
    pa &= ~(PAGE_SIZE - 1);

    va = mem_alloc(&cio, npages * PAGE_SIZE, PAGE_SIZE);
    if (va == 0)
        return va;

    mva = (unsigned int) va;
    //printk("map_io: va 0x%p pa 0x%x off 0x%x npages %d\n", va, pa, off, npages); /* P3 */
    while (npages-- != 0) {
        map_page(mva, pa, 1);
        mva += PAGE_SIZE;
        pa += PAGE_SIZE;
    }

    return (void *)((unsigned int)va + off);
}

/*
 * Switch page tables.
 */
void
init_mmu_swift()
{
    unsigned int addr, i;
    unsigned long pa, va;

    mem_init(&cmem, (char *) &_vmem, (char *)&_evmem);
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

    // 1:1 mapping for ROM
    pa = va = (unsigned long)&_start;
    for (; va < (unsigned long)&_data; va += PAGE_SIZE, pa += PAGE_SIZE) {
        map_page(va, pa, 0);
    }

    // data & bss mapped to end of RAM
    va = (unsigned long)&_data;
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
    iommu_init(&ciommu);
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
iommu_init(struct iommu *t)
{
    unsigned int *ptab;
    int ptsize;
    struct iommu_regs *regs =NULL;
    unsigned int impl, vers;
    unsigned int tmp;

    if ((regs = map_io(PHYS_JJ_IOMMU, sizeof(struct iommu_regs))) == 0) {
        printk("Cannot map IOMMU\n");
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
        printk("Cannot allocate IOMMU table [0x%x]\n", ptsize);
        for (;;) { }
    }
    t->page_table = ptab;

    /* flush_cache_all(); */
    /** flush_tlb_all(); **/
    regs->base = ((unsigned int)va2pa((unsigned long)ptab)) >> 4;
    iommu_invalidate(regs);

    printk("IOMMU: impl %d vers %d page table at 0x%p of size %d bytes\n",
           impl, vers, t->page_table, ptsize);

    mem_init(&t->bmap, (char*)t->plow, (char *)0xfffff000);
}
