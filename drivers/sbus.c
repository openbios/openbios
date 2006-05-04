/*
 *   OpenBIOS SBus driver
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
ob_sbus_open(int *idx)
{
	int ret=1;
	RET ( -ret );
}

static void
ob_sbus_close(int *idx)
{
	selfword("close-deblocker");
}

static void
ob_sbus_initialize(int *idx)
{
}


NODE_METHODS(ob_sbus_node) = {
	{ NULL,			ob_sbus_initialize	},
	{ "open",		ob_sbus_open		},
	{ "close",		ob_sbus_close		},
};


int ob_sbus_init(void)
{
	printk("Initializing SBus devices...\n");
	
	return 0;
}
