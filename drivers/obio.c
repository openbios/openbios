/*
 *   OpenBIOS Sparc OBIO driver
 *   
 *   (C) 2004 Stefan Reinauer <stepan@openbios.org>
 *   (C) 2005 Ed Schouten <ed@fxq.nl>
 * 
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/kernel.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"

#include "openbios/drivers.h"

static void
ob_zs_init(unsigned long slavio_base)
{
    unsigned long zs_addr;

    zs_addr = (unsigned long)map_io(slavio_base + 0x0, 8);
    push_str("/obio/zs@0,0");
    fword("find-device");
    
    PUSH(zs_addr);
    fword("encode-int");
    PUSH(sizeof(zs_addr));
    fword("encode-int");
    fword("encode+");
    push_str("address");
    fword("property");

    zs_addr = (unsigned long)map_io(slavio_base + 0x00100000, 8);
    push_str("/obio/zs@0,100000");
    fword("find-device");
    
    PUSH(zs_addr);
    fword("encode-int");
    PUSH(sizeof(zs_addr));
    fword("encode-int");
    fword("encode+");
    push_str("address");
    fword("property");
}

static void
ob_obio_open(int *idx)
{
	int ret=1;
	RET ( -ret );
}

static void
ob_obio_close(int *idx)
{
	selfword("close-deblocker");
}

static void
ob_obio_initialize(int *idx)
{
}


NODE_METHODS(ob_obio_node) = {
	{ NULL,			ob_obio_initialize	},
	{ "open",		ob_obio_open		},
	{ "close",		ob_obio_close		},
};


int ob_obio_init(unsigned long slavio_base)
{

    //printk("Initializing OBIO devices...\n");
    
    ob_zs_init(slavio_base);
	
    return 0;
}
