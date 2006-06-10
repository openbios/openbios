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

#define SBUS_REGS        0x28
#define SBUS_SLOTS       5

static void
ob_sbus_node_init(unsigned long base)
{
    void *regs;

    push_str("/iommu/sbus");
    fword("find-device");

    PUSH(0);
    fword("encode-int");
    PUSH(base);
    fword("encode-int");
    fword("encode+");
    PUSH(SBUS_REGS);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    regs = map_io(base, SBUS_REGS);
    PUSH((unsigned long)regs);
    fword("encode-int");
    push_str("address");
    fword("property");
}

static void
ob_macio_init(unsigned int slot, unsigned long base)
{
    // All devices were integrated to NCR89C100, see
    // http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR89C100.txt

    // NCR 53c9x, aka ESP. See
    // http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR53C9X.txt
#ifdef CONFIG_DRIVER_ESP
    ob_esp_init(base);
#endif

    // NCR 92C990, Am7990, Lance. See http://www.amd.com
    //ob_le_init(base);

    // Parallel port
    //ob_bpp_init(base);

    // Power management
    //ob_power_init(base);
}

static void
sbus_probe_slot(unsigned int slot, unsigned long base)
{
    // OpenBIOS and Qemu don't know how to do Sbus probing
    switch(slot) {
    case 2: // SUNW,tcx
        //ob_tcx_init(slot, base);
        break;
    case 3: // SUNW,CS4231
        //ob_cs4231_init(slot, base);
        break;
    case 4: // MACIO: le, esp, bpp, power-management
        ob_macio_init(slot, base);
        break;
    default:
        break;
    }
}

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

static const unsigned long sbus_offset[SBUS_SLOTS] = {
    0x30000000,
    0x40000000,
    0x50000000,
    0x60000000,
    0x70000000,
};

int ob_sbus_init(unsigned long base)
{
    unsigned int slot;

    ob_sbus_node_init(base);

    for (slot = 0; slot < SBUS_SLOTS; slot++) {
        sbus_probe_slot(slot, sbus_offset[slot]);
    }

    return 0;
}
