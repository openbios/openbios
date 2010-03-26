/* tag: forth source loader
 *
 * Copyright (C) 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "config.h"
#include "kernel/kernel.h"
#include "libopenbios/bindings.h"
#include "libopenbios/sys_info.h"
#include "libc/diskio.h"
#include "boot.h"
#define printk printk
#define debug printk

static char *forthtext=NULL;
static int fd;

int forth_load(const char *filename)
{
    char magic[2];
    unsigned long forthsize;
    int retval = -1;

    fd = open_io(filename);
    if (!fd)
	goto out;

    if (read_io(fd, magic, 2) != 2) {
	debug("Can't read magic header\n");
	retval = LOADER_NOT_SUPPORT;
	goto out;
    }

    if (magic[0] != '\\' || magic[1] != ' ') {
	debug("No forth source image\n");
	retval = LOADER_NOT_SUPPORT;
	goto out;
    }

    /* Calculate the file size by seeking to the end of the file */
    seek_io(fd, -1);
    forthsize = tell(fd);
    forthtext = malloc(forthsize+1);
    seek_io(fd, 0);

    printk("Loading forth source ...");
    if ((unsigned long)read_io(fd, forthtext, forthsize) != forthsize) {
	printk("Can't read forth text\n");
	goto out;
    }
    forthtext[forthsize]=0;
    printk("ok\n");

    close_io(fd);

    PUSH ( (ucell)forthtext );
    PUSH ( (ucell)forthsize );
    fword("eval2");
    retval=0;

out:
    //if (forthtext)
    //	free(forthtext);
    return retval;
}
