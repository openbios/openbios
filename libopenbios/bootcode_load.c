/* 
 * Raw bootcode loader (CHRP/Apple %BOOT)
 * Written by Mark Cave-Ayland 2013
 */

#include "config.h"
#include "kernel/kernel.h"
#include "libopenbios/bindings.h"
#include "libopenbios/bootcode_load.h"
#include "libc/diskio.h"
#include "drivers/drivers.h"
#define printf printk
#define debug printk

#define OLDWORLD_BOOTCODE_BASEADDR	(0x3f4000)

int 
bootcode_load(ihandle_t dev)
{
    int retval = -1, count = 0, fd;
    unsigned long bootcode, loadbase, offset;

    /* Mark the saved-program-state as invalid */
    feval("0 state-valid !");

    fd = open_ih(dev);
    if (fd == -1) {
        goto out;
    }

    /* Default to loading at load-base */
    fword("load-base");
    loadbase = POP();
    
#ifdef CONFIG_PPC
    /* However Old World Macs need to load to a different address */
    if (is_oldworld()) {
        loadbase = OLDWORLD_BOOTCODE_BASEADDR;
    }
#endif
    
    bootcode = loadbase;
    offset = 0;
    
    while(1) {
        if (seek_io(fd, offset) == -1)
            break;
        count = read_io(fd, (void *)bootcode, 512);
        offset += count;
        bootcode += count;
    }

    /* If we didn't read anything then exit */
    if (!count) {
        goto out;
    }
    
    /* Initialise saved-program-state */
    PUSH(loadbase);
    feval("saved-program-state >sps.entry !");
    PUSH(offset);
    feval("saved-program-state >sps.file-size !");
    feval("bootcode saved-program-state >sps.file-type !");

    feval("-1 state-valid !");

out:
    close_io(fd);
    return retval;
}

