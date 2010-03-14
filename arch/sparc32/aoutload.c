/* a.out boot loader
 * As we have seek, this implementation can be straightforward.
 * 2003-07 by SONE Takeshi
 */

#include "openbios/config.h"
#include "kernel/kernel.h"
#include "a.out.h"
#include "sys_info.h"
#include "loadfs.h"
#include "boot.h"
#define printf printk
#define debug printk

#define addr_fixup(addr) ((addr) & 0x00ffffff)

static char *image_name, *image_version;

static int check_mem_ranges(struct sys_info *info,
                            unsigned long start,
                            unsigned long size)
{
    int j;
    unsigned long end;
    unsigned long prog_start, prog_end;
    struct memrange *mem;

    prog_start = virt_to_phys(&_start);
    prog_end = virt_to_phys(&_end);

    end = start + size;

    if (start < prog_start && end > prog_start)
        goto conflict;
    if (start < prog_end && end > prog_end)
        goto conflict;
    mem = info->memrange;
    for (j = 0; j < info->n_memranges; j++) {
        if (mem[j].base <= start && mem[j].base + mem[j].size >= end)
            break;
    }
    if (j >= info->n_memranges)
        goto badseg;
    return 1;

 conflict:
    printf("%s occupies [%#lx-%#lx]\n", program_name, prog_start, prog_end);

 badseg:
    printf("A.out file [%#lx-%#lx] doesn't fit into memory\n", start, end - 1);
    return 0;
}

int aout_load(struct sys_info *info, const char *filename, const void *romvec)
{
    int retval = -1;
    int image_retval;
    struct exec ehdr;
    unsigned long start, size;
    unsigned int offset = 512;

    image_name = image_version = NULL;

    if (!file_open(filename))
	goto out;

    file_seek(offset);

    if (lfile_read(&ehdr, sizeof ehdr) != sizeof ehdr) {
        debug("Can't read a.out header\n");
        retval = LOADER_NOT_SUPPORT;
        goto out;
    }

    if (N_BADMAG(ehdr)) {
	debug("Not a bootable a.out image\n");
	retval = LOADER_NOT_SUPPORT;
	goto out;
    }

    if (ehdr.a_text == 0x30800007)
	ehdr.a_text=64*1024;

    if (N_MAGIC(ehdr) == NMAGIC) {
        size = addr_fixup(N_DATADDR(ehdr)) + addr_fixup(ehdr.a_data);
    } else {
        size = addr_fixup(ehdr.a_text) + addr_fixup(ehdr.a_data);
    }

    if (size < 7680)
        size = 7680;


    start = 0x4000; // N_TXTADDR(ehdr);

    if (!check_mem_ranges(info, start, size))
	goto out;

    printf("Loading a.out %s...\n", image_name ? image_name : "image");

    file_seek(offset + N_TXTOFF(ehdr));

    if (N_MAGIC(ehdr) == NMAGIC) {
        if ((unsigned long)lfile_read((void *)start, ehdr.a_text) != ehdr.a_text) {
            printf("Can't read program text segment (size 0x%lx)\n", ehdr.a_text);
            goto out;
        }
        if ((unsigned long)lfile_read((void *)(start + N_DATADDR(ehdr)), ehdr.a_data) != ehdr.a_data) {
            printf("Can't read program data segment (size 0x%lx)\n", ehdr.a_data);
            goto out;
        }
    } else {
        if ((unsigned long)lfile_read((void *)start, size) != size) {
            printf("Can't read program (size 0x%lx)\n", size);
            goto out;
        }
    }

    debug("Loaded %lu bytes\n", size);

    debug("entry point is %#lx\n", start);
    printf("Jumping to entry point...\n");

#if 1
    {
        int (*entry)(const void *romvec_ptr, int p2, int p3, int p4, int p5);

        entry = (void *) addr_fixup(start);
        image_retval = entry(romvec, 0, 0, 0, 0);
    }
#endif

    printf("Image returned with return value %#x\n", image_retval);
    retval = 0;

out:
    file_close();
    return retval;
}
