/* lib.c
 * tag: simple function library
 *
 * Copyright (C) 2003 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "openbios/config.h"
#include "libc/vsprintf.h"
#include "openbios/bindings.h"
#include "spitfire.h"
#include "sys_info.h"
#include "boot.h"

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

#define MEMSIZE ((128 + 256 + 512) * 1024)
static char memory[MEMSIZE];
static void *memptr=memory;
static int memsize=MEMSIZE;


void *malloc(int size)
{
	void *ret=(void *)0;

	if( !size )
		return NULL;

        size = (size + 7) & ~7;

	if(memsize>=size) {
		memsize-=size;
		ret=memptr;
		memptr = (void *)((unsigned long)memptr + size);
	}
	return ret;
}

void free(void *ptr)
{
	/* Nothing yet */
}

#define PAGE_SIZE_4M   (4 * 1024 * 1024)
#define PAGE_SIZE_512K (512 * 1024)
#define PAGE_SIZE_64K  (64 * 1024)
#define PAGE_SIZE_8K   (8 * 1024)
#define PAGE_MASK_4M   (4 * 1024 * 1024 - 1)
#define PAGE_MASK_512K (512 * 1024 - 1)
#define PAGE_MASK_64K  (64 * 1024 - 1)
#define PAGE_MASK_8K   (8 * 1024 - 1)

static void
mmu_open(void)
{
    RET(-1);
}

static void
mmu_close(void)
{
}

static int
spitfire_translate(unsigned long virt, unsigned long *p_phys,
                   unsigned long *p_data, unsigned long *p_size)
{
    unsigned long phys, tag, data, mask, size;
    unsigned int i;

    for (i = 0; i < 64; i++) {
        data = spitfire_get_dtlb_data(i);
        if (data & 0x8000000000000000ULL) { // Valid entry?
            switch ((data >> 61) & 3) {
            default:
            case 0x0: // 8k
                mask = 0xffffffffffffe000ULL;
                size = PAGE_SIZE_8K;
                break;
            case 0x1: // 64k
                mask = 0xffffffffffff0000ULL;
                size = PAGE_SIZE_64K;
                break;
            case 0x2: // 512k
                mask = 0xfffffffffff80000ULL;
                size = PAGE_SIZE_512K;
                break;
            case 0x3: // 4M
                mask = 0xffffffffffc00000ULL;
                size = PAGE_SIZE_4M;
                break;
            }
            tag = spitfire_get_dtlb_tag(i);
            if ((virt & mask) == (tag & mask)) {
                phys = data & mask & 0x000001fffffff000ULL;
                phys |= virt & ~mask;
                *p_phys = phys;
                *p_data = data & 0xfff;
                *p_size = size;
                return -1;
            }
        }
    }

    return 0;
}

/*
  3.6.5 translate
  ( virt -- false | phys.lo ... phys.hi mode true )
*/
static void
mmu_translate(void)
{
    unsigned long virt, phys, data, size;

    virt = POP();

    if (spitfire_translate(virt, &phys, &data, &size)) {
        PUSH(phys & 0xffffffff);
        PUSH(phys >> 32);
        PUSH(data);
        PUSH(-1);
        return;
    }
    PUSH(0);
}

static void
dtlb_load2(unsigned long vaddr, unsigned long tte_data)
{
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%%g0] %4\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_DMMU),
          "r" (tte_data), "i" (ASI_DTLB_DATA_IN));
}

static void
dtlb_load3(unsigned long vaddr, unsigned long tte_data,
           unsigned long tte_index)
{
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%4] %5\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_DMMU),
          "r" (tte_data), "r" (tte_index << 3), "i" (ASI_DTLB_DATA_ACCESS));
}

/*
  ( index tte_data vaddr -- ? )
*/
static void
dtlb_load(void)
{
    unsigned long vaddr, tte_data, idx;

    vaddr = POP();
    tte_data = POP();
    idx = POP();
    dtlb_load3(vaddr, tte_data, idx);
}

static void
itlb_load2(unsigned long vaddr, unsigned long tte_data)
{
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%%g0] %4\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_IMMU),
          "r" (tte_data), "i" (ASI_ITLB_DATA_IN));
}

static void
itlb_load3(unsigned long vaddr, unsigned long tte_data,
           unsigned long tte_index)
{
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%4] %5\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_IMMU),
          "r" (tte_data), "r" (tte_index << 3), "i" (ASI_ITLB_DATA_ACCESS));
}

/*
  ( index tte_data vaddr -- ? )
*/
static void
itlb_load(void)
{
    unsigned long vaddr, tte_data, idx;

    vaddr = POP();
    tte_data = POP();
    idx = POP();
    itlb_load3(vaddr, tte_data, idx);
}

static void
map_pages(unsigned long virt, unsigned long size, unsigned long phys)
{
    unsigned long tte_data, currsize;

    size = (size + PAGE_MASK_8K) & ~PAGE_MASK_8K;
    while (size >= PAGE_SIZE_8K) {
        currsize = size;
        if (currsize >= PAGE_SIZE_4M &&
            (virt & PAGE_MASK_4M) == 0 &&
            (phys & PAGE_MASK_4M) == 0) {
            currsize = PAGE_SIZE_4M;
            tte_data = 6ULL << 60;
        } else if (currsize >= PAGE_SIZE_512K &&
                   (virt & PAGE_MASK_512K) == 0 &&
                   (phys & PAGE_MASK_512K) == 0) {
            currsize = PAGE_SIZE_512K;
            tte_data = 4ULL << 60;
        } else if (currsize >= PAGE_SIZE_64K &&
                   (virt & PAGE_MASK_64K) == 0 &&
                   (phys & PAGE_MASK_64K) == 0) {
            currsize = PAGE_SIZE_64K;
            tte_data = 2ULL << 60;
        } else {
            currsize = PAGE_SIZE_8K;
            tte_data = 0;
        }
        tte_data |= phys | 0x8000000000000036ULL;
        dtlb_load2(virt, tte_data);
        itlb_load2(virt, tte_data);
        size -= currsize;
        phys += currsize;
        virt += currsize;
    }
}

/*
  3.6.5 map
  ( phys.lo ... phys.hi virt size mode -- )
*/
static void
mmu_map(void)
{
    unsigned long virt, size, mode, phys;

    mode = POP();
    size = POP();
    virt = POP();
    phys = POP();
    phys <<= 32;
    phys |= POP();
    map_pages(virt, size, phys);
}

static void
itlb_demap(unsigned long vaddr)
{
    asm("stxa %0, [%0] %1\n"
        : : "r" (vaddr), "i" (ASI_IMMU_DEMAP));
}

static void
dtlb_demap(unsigned long vaddr)
{
    asm("stxa %0, [%0] %1\n"
        : : "r" (vaddr), "i" (ASI_DMMU_DEMAP));
}

static void
unmap_pages(unsigned long virt, unsigned long size)
{
    unsigned long phys, data;

    unsigned long currsize;

    // align size
    size = (size + PAGE_MASK_8K) & ~PAGE_MASK_8K;

    while (spitfire_translate(virt, &phys, &data, &currsize)) {

        itlb_demap(virt & ~0x1fffULL);
        dtlb_demap(virt & ~0x1fffULL);

        size -= currsize;
        virt += currsize;
    }
}

/*
  3.6.5 unmap
  ( virt size -- )
*/
static void
mmu_unmap(void)
{
    unsigned long virt, size;

    size = POP();
    virt = POP();
    unmap_pages(virt, size);
}

/*
  3.6.5 claim
  ( virt size align -- base )
*/
static void
mmu_claim(void)
{
    unsigned long virt, size, align;

    align = POP();
    size = POP();
    virt = POP();
    printk("claim virt = %lx size = %lx align = %lx\n", virt, size, align);
    PUSH(virt); // XXX
}

/*
  3.6.5 release
  ( virt size -- )
*/
static void
mmu_release(void)
{
    unsigned long virt, size;

    size = POP();
    virt = POP();
    printk("release virt = %lx size = %lx\n", virt, size);
    // XXX
}

DECLARE_UNNAMED_NODE(mmu, INSTALL_OPEN, 0);

NODE_METHODS(mmu) = {
    { "open",               mmu_open              },
    { "close",              mmu_close             },
    { "translate",          mmu_translate         },
    { "SUNW,dtlb-load",     dtlb_load             },
    { "SUNW,itlb-load",     itlb_load             },
    { "map",                mmu_map               },
    { "unmap",              mmu_unmap             },
    { "claim",              mmu_claim             },
    { "release",            mmu_release           },
};

void ob_mmu_init(const char *cpuname, uint64_t ram_size)
{
    char nodebuff[256];

    // MMU node
    snprintf(nodebuff, sizeof(nodebuff), "/%s", cpuname);
    push_str(nodebuff);
    fword("find-device");

    fword("new-device");

    push_str("mmu");
    fword("device-name");

    fword("finish-device");

    snprintf(nodebuff, sizeof(nodebuff), "/%s/mmu", cpuname);

    REGISTER_NODE_METHODS(mmu, nodebuff);

    push_str("/chosen");
    fword("find-device");

    push_str(nodebuff);
    fword("open-dev");
    fword("encode-int");
    push_str("mmu");
    fword("property");

    push_str("/memory");
    fword("find-device");

    // All memory: 0 to RAM_size
    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((int)(ram_size >> 32));
    fword("encode-int");
    fword("encode+");
    PUSH((int)(ram_size & 0xffffffff));
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    // Available memory: 0 to va2pa(_start)
    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((va2pa((unsigned long)&_data) - 8192) >> 32);
    fword("encode-int");
    fword("encode+");
    PUSH((va2pa((unsigned long)&_data) - 8192) & 0xffffffff);
    fword("encode-int");
    fword("encode+");
    push_str("available");
    fword("property");

    // XXX
    // Translations
    push_str("/virtual-memory");
    fword("find-device");

    // 0 to 16M: 1:1
    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(16 * 1024 * 1024);
    fword("encode-int");
    fword("encode+");
    PUSH(0x80000000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00000036);
    fword("encode-int");
    fword("encode+");

    // _start to _data: ROM used
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_start);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_data - (unsigned long)&_start);
    fword("encode-int");
    fword("encode+");
    PUSH(0x800001ff);
    fword("encode-int");
    fword("encode+");
    PUSH(0xf0000074);
    fword("encode-int");
    fword("encode+");

    // _data to _end: end of RAM
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_data);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_data - (unsigned long)&_start);
    fword("encode-int");
    fword("encode+");
    PUSH(((va2pa((unsigned long)&_data) - 8192) >> 32) | 0x80000000);
    fword("encode-int");
    fword("encode+");
    PUSH(((va2pa((unsigned long)&_data) - 8192) & 0xffffffff) | 0x36);
    fword("encode-int");
    fword("encode+");

    // VGA buffer (128k): 1:1
    PUSH(0x1ff);
    fword("encode-int");
    fword("encode+");
    PUSH(0x004a0000);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(128 * 1024);
    fword("encode-int");
    fword("encode+");
    PUSH(0x800001ff);
    fword("encode-int");
    fword("encode+");
    PUSH(0x004a0076);
    fword("encode-int");
    fword("encode+");

    push_str("translations");
    fword("property");

    push_str("/openprom/client-services");
    fword("find-device");
    bind_func("claim", mmu_claim);
    bind_func("release", mmu_release);
}
