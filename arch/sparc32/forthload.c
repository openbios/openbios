/* tag: forth source loader
 *
 * Copyright (C) 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "openbios/config.h"
#include "openbios/kernel.h"
#include "openbios/bindings.h"
#include "sys_info.h"
#include "loadfs.h"
#include "boot.h"
#define printk printk
#define debug printk

static char *forthtext=NULL;

int forth_load(const char *filename)
{
    char magic[2];
    unsigned long forthsize;
    int retval = -1;

    if (!file_open(filename))
	goto out;

    if (lfile_read(magic, 2) != 2) {
	debug("Can't read magic header\n");
	retval = LOADER_NOT_SUPPORT;
	goto out;
    }

    if (magic[0] != '\\' || magic[1] != ' ') {
	debug("No forth source image\n");
	retval = LOADER_NOT_SUPPORT;
	goto out;
    }

    forthsize = file_size();

    forthtext = malloc(forthsize+1);
    file_seek(0);

    printk("Loading forth source ...");
    if ((unsigned long)lfile_read(forthtext, forthsize) != forthsize) {
	printk("Can't read forth text\n");
	goto out;
    }
    forthtext[forthsize]=0;
    printk("ok\n");

    PUSH ( (ucell)forthtext );
    PUSH ( (ucell)forthsize );
    fword("eval2");
    retval=0;

out:
    //if (forthtext)
    //	free(forthtext);
    return retval;
}
