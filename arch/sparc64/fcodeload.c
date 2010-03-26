/*
 * FCode boot loader
 */

#include "config.h"
#include "kernel/kernel.h"
#include "libopenbios/bindings.h"
#include "libopenbios/sys_info.h"
#include "libc/diskio.h"
#include "boot.h"
#define printf printk
#define debug printk

static int fd;

int fcode_load(const char *filename)
{
    int retval = -1;
    uint8_t fcode_header[8];
    unsigned long start, size;
    unsigned int offset;

    /* Mark the saved-program-state as invalid */
    feval("0 state-valid !");

    fd = open_io(filename);
    if (!fd)
        goto out;

    for (offset = 0; offset < 16 * 512; offset += 512) {
        seek_io(fd, offset);
        if (read_io(fd, &fcode_header, sizeof(fcode_header))
            != sizeof(fcode_header)) {
            debug("Can't read FCode header from file %s\n", filename);
            retval = LOADER_NOT_SUPPORT;
            goto out;
        }
        switch (fcode_header[0]) {
        case 0xf0: // start0
        case 0xf1: // start1
        case 0xf2: // start2
        case 0xf3: // start4
        case 0xfd: // version1
            goto found;
        }
    }

    debug("Not a bootable FCode image\n");
    retval = LOADER_NOT_SUPPORT;
    goto out;

 found:
    size = (fcode_header[4] << 24) | (fcode_header[5] << 16) |
        (fcode_header[6] << 8) | fcode_header[7];

    start = 0x4000;

    printf("Loading FCode image...\n");

    seek_io(fd, offset + sizeof(fcode_header));

    if ((unsigned long)read_io(fd, (void *)start, size) != size) {
        printf("Can't read file (size 0x%lx)\n", size);
        goto out;
    }

    debug("Loaded %lu bytes\n", size);
    debug("entry point is %#lx\n", start);

    // Initialise saved-program-state
    PUSH(start);
    feval("saved-program-state >sps.entry !");
    PUSH(size);
    feval("saved-program-state >sps.file-size !");
    feval("fcode saved-program-state >sps.file-type !");

    printf("Evaluating FCode...\n");

    PUSH(start);
    PUSH(1);
    fword("byte-load");

    retval = 0;

out:
    close_io(fd);
    return retval;
}
