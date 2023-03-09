/* Support for Multiboot */

#include "config.h"
#include "asm/io.h"
#include "libopenbios/sys_info.h"
#include "multiboot.h"

#define printf printk
#ifdef CONFIG_DEBUG_BOOT
#define debug printk
#else
#define debug(x...)
#endif

struct mbheader {
    unsigned int magic, flags, checksum;
};
const struct mbheader multiboot_header
	__attribute__((section (".hdr"))) =
{
    MULTIBOOT_HEADER_MAGIC,
    MULTIBOOT_HEADER_FLAGS,
    -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
};

/* Multiboot information structure, provided by loader to us */

struct multiboot_mmap {
	unsigned entry_size;
	unsigned base_lo, base_hi;
	unsigned size_lo, size_hi;
	unsigned type;
};

#define MULTIBOOT_MEM_VALID       0x01
#define MULTIBOOT_BOOT_DEV_VALID  0x02
#define MULTIBOOT_CMDLINE_VALID   0x04
#define MULTIBOOT_MODS_VALID      0x08
#define MULTIBOOT_AOUT_SYMS_VALID 0x10
#define MULTIBOOT_ELF_SYMS_VALID  0x20
#define MULTIBOOT_MMAP_VALID      0x40

void collect_multiboot_info(struct sys_info *info);
void collect_multiboot_info(struct sys_info *info)
{
    struct multiboot_info *mbinfo;
    struct multiboot_mmap *mbmem;
    unsigned mbcount, mbaddr;
    int i;
    struct memrange *mmap;
    int mmap_count;
    module_t *mod;

    if (info->boot_type != 0x2BADB002)
	return;

    debug("Using Multiboot information at %#lx\n", info->boot_data);

    mbinfo = phys_to_virt(info->boot_data);

    if (mbinfo->mods_count != 1) {
	    printf("Multiboot: no dictionary\n");
	    return;
    }

    mod = (module_t *) mbinfo->mods_addr;
    info->dict_start=(unsigned long *)mod->mod_start;
    info->dict_end=(unsigned long *)mod->mod_end;

    void collect_multiboot_info(struct sys_info *info) {
    if (info->boot_type != 0x2BADB002)
        return;

    debug("Using Multiboot information at %#lx\n", info->boot_data);

    struct multiboot_info *mbinfo = phys_to_virt(info->boot_data);

    if (mbinfo->mods_count != 1) {
        printf("Multiboot: no dictionary\n");
        return;
    }

    module_t *mod = (module_t *)mbinfo->mods_addr;
    info->dict_start = (unsigned long *)mod->mod_start;
    info->dict_end = (unsigned long *)mod->mod_end;

    if (mbinfo->flags & MULTIBOOT_MMAP_VALID) {
        // count the number of entries in the memory map
        unsigned mmap_count = 0;
        struct multiboot_mmap *mbmem = phys_to_virt(mbinfo->mmap_addr);
        for (unsigned char *p = (unsigned char *)mbinfo->mmap_addr;
             p < (unsigned char *)mbinfo->mmap_addr + mbinfo->mmap_length;
             p += mbmem->entry_size + 4) {
            mbmem = (struct multiboot_mmap *)p;
            if (mbmem->type == 1) // Only normal RAM
                mmap_count++;
        }

        // allocate memory for the memory map
        struct memrange *mmap = malloc(mmap_count * sizeof(struct memrange));
        if (mmap == NULL) {
            printf("Failed to allocate memory for memory map\n");
            return;
        }

        // fill the memory map
        mbmem = phys_to_virt(mbinfo->mmap_addr);
        unsigned i = 0;
        for (unsigned char *p = (unsigned char *)mbinfo->mmap_addr;
             p < (unsigned char *)mbinfo->mmap_addr + mbinfo->mmap_length;
             p += mbmem->entry_size + 4) {
            mbmem = (struct multiboot_mmap *)p;
            if (mbmem->type == 1) { // Only normal RAM
                mmap[i].base = mbmem->base_lo + ((unsigned long long)mbmem->base_hi << 32);
                mmap[i].size = mbmem->size_lo + ((unsigned long long)mbmem->size_hi << 32);
                i++;
            }
        }

        if (i < 2) {
            printf("Multiboot mmap is broken\n");
            free(mmap);
            return;
        }

        info->memrange = mmap;
        info->n_memranges = i;
    } else if (mbinfo->flags & MULTIBOOT_MEM_VALID) {
        // use mem_lower and mem_upper
        struct memrange *mmap = malloc(2 * sizeof(*mmap));
        if (mmap == NULL) {
            printf("Failed to allocate memory for memory map\n");
            return;
        }

        mmap[0].base = 0;
        mmap[0].size = mbinfo->mem_lower << 10;
        mmap[1].base = 1 << 20; // 1MB
        mmap[1].size = mbinfo->mem_upper << 10;

        info->memrange = mmap;
        info->n_memranges = 2;
    } else {
        printf("Can't get memory information from Multiboot\n");
    }
}
	free(mmap);
	/* fall back to mem_lower/mem_upper */
    }

    if (mbinfo->flags & MULTIBOOT_MEM_VALID) {
	/* use mem_lower and mem_upper */
	mmap_count = 2;
	mmap = malloc(2 * sizeof(*mmap));
	mmap[0].base = 0;
	mmap[0].size = mbinfo->mem_lower << 10;
	mmap[1].base = 1 << 20; /* 1MB */
	mmap[1].size = mbinfo->mem_upper << 10;
	goto got_it;
    }

    printf("Can't get memory information from Multiboot\n");
    return;

got_it:
    info->memrange = mmap;
    info->n_memranges = mmap_count;

    return;
}
