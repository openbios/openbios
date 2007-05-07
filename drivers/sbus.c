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
#define SBUS_SLOTS       16
#define POWER_REGS       0x10
#define POWER_OFFSET     0x0a000000

static void
ob_sbus_node_init(unsigned long bus, unsigned long base)
{
    void *regs;

    push_str("/iommu/sbus");
    fword("find-device");

    PUSH(bus);
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
ob_le_init(unsigned int slot, unsigned long base, unsigned long offset)
{
    push_str("/iommu/sbus/ledma");
    fword("find-device");
    PUSH(slot);
    fword("encode-int");
    PUSH(offset + 0x00400010);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00000020);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    push_str("/iommu/sbus/ledma/le");
    fword("find-device");
    PUSH(slot);
    fword("encode-int");
    PUSH(offset + 0x00c00000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00000004);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");
}

uint16_t graphic_depth;

static void
ob_tcx_init(unsigned int slot, unsigned long base)
{
    push_str("/iommu/sbus/SUNW,tcx");
    fword("find-device");

    PUSH(slot);
    fword("encode-int");
    PUSH(0x00800000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00100000);
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x02000000);
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00400000);
    } else {
        PUSH(0x00000001);
    }
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x04000000);
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00400000);
    } else {
        PUSH(0x00000001);
    }
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x06000000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00800000);
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x0a000000);
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00400000);
    } else {
        PUSH(0x00000001);
    }
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x0c000000);
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00800000);
    } else {
        PUSH(0x00000001);
    }
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x0e000000);
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00800000);
    } else {
        PUSH(0x00000001);
    }
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00700000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00001000);
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00200000);
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00004000);
    } else {
        PUSH(0x00000004);
    }
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00301000);
    } else {
        PUSH(0x00300000);
    }
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00001000);
    } else {
        PUSH(0x0000081c);
    }
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00000000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00010000);
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00240000);
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00004000);
    } else {
        PUSH(0x00000004);
    }
    fword("encode-int");
    fword("encode+");

    PUSH(slot);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00280000);
    fword("encode-int");
    fword("encode+");
    if (graphic_depth == 24) {
        PUSH(0x00008000);
    } else {
        PUSH(0x00000001);
    }
    fword("encode-int");
    fword("encode+");

    push_str("reg");
    fword("property");

    PUSH((int)graphic_depth);
    fword("encode-int");
    push_str("depth");
    fword("property");

    if (graphic_depth == 8) {
        push_str("true");
        fword("encode-string");
        push_str("tcx-8-bit");
        fword("property");
    }
}

static void
ob_power_init(unsigned int slot, unsigned long base)
{
    push_str("/iommu/sbus");
    fword("find-device");
    fword("new-device");

    push_str("power-management");
    fword("device-name");

    PUSH(slot);
    fword("encode-int");
    PUSH(base);
    fword("encode-int");
    fword("encode+");
    PUSH(POWER_REGS);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    fword("finish-device");
}

static void
ob_macio_init(unsigned int slot, unsigned long base, unsigned long offset)
{
    // All devices were integrated to NCR89C100, see
    // http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR89C100.txt

    // NCR 53c9x, aka ESP. See
    // http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR53C9X.txt
#ifdef CONFIG_DRIVER_ESP
    ob_esp_init(slot, base, offset);
#endif

    // NCR 92C990, Am7990, Lance. See http://www.amd.com
    ob_le_init(slot, base, offset);

    // Parallel port
    //ob_bpp_init(base);

    // Power management
    ob_power_init(slot, POWER_OFFSET);
}

static void
sbus_probe_slot_ss5(unsigned int slot, unsigned long base)
{
    // OpenBIOS and Qemu don't know how to do Sbus probing
    switch(slot) {
    case 3: // SUNW,tcx
        ob_tcx_init(slot, base);
        break;
    case 4: // SUNW,CS4231
        //ob_cs4231_init(slot, base);
        break;
    case 5: // MACIO: le, esp, bpp, power-management
        ob_macio_init(slot, base, 0x08000000);
        break;
    default:
        break;
    }
}

static void
sbus_probe_slot_ss10(unsigned int slot, unsigned long base)
{
    // OpenBIOS and Qemu don't know how to do Sbus probing
    switch(slot) {
    case 2: // SUNW,tcx
        ob_tcx_init(slot, base);
        break;
    case 0xf: // le, esp, bpp, power-management
        ob_macio_init(slot, base, 0);
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

static const unsigned long sbus_offsets_ss5[SBUS_SLOTS][5] = {
    { 0, 0, 0x0, 0x20000000, 0x10000000,},
    { 1, 0, 0x0, 0x30000000, 0x10000000,},
    { 2, 0, 0x0, 0x40000000, 0x10000000,},
    { 3, 0, 0x0, 0x50000000, 0x10000000,},
    { 4, 0, 0x0, 0x60000000, 0x10000000,},
    { 5, 0, 0x0, 0x70000000, 0x10000000,},
};

static const unsigned long sbus_offsets_ss10[SBUS_SLOTS][5] = {
    { 0, 0, 0xe, 0x00000000, 0x10000000,},
    { 1, 0, 0xe, 0x10000000, 0x10000000,},
    { 2, 0, 0xe, 0x20000000, 0x10000000,},
    { 3, 0, 0xe, 0x30000000, 0x10000000,},
    [0xf] = { 0xf, 0, 0xe, 0xf0000000, 0x10000000,},
};

static void
ob_add_sbus_range(const unsigned long *range, int notfirst)
{
    if (!notfirst) {
        push_str("/iommu/sbus");
        fword("find-device");
    }
    PUSH(range[0]);
    fword("encode-int");
    if (notfirst)
        fword("encode+");
    PUSH(range[1]);
    fword("encode-int");
    fword("encode+");
    PUSH(range[2]);
    fword("encode-int");
    fword("encode+");
    PUSH(range[3]);
    fword("encode-int");
    fword("encode+");
    PUSH(range[4]);
    fword("encode-int");
    fword("encode+");
}

static int
ob_sbus_init_ss5(unsigned long bus, unsigned long base)
{
    unsigned int slot;
    int notfirst = 0;

    for (slot = 0; slot < SBUS_SLOTS; slot++) {
        if (sbus_offsets_ss5[slot][4] > 0)
            ob_add_sbus_range(sbus_offsets_ss5[slot], notfirst++);
    }
    push_str("ranges");
    fword("property");

    for (slot = 0; slot < SBUS_SLOTS; slot++) {
        if (sbus_offsets_ss5[slot][4] > 0)
            sbus_probe_slot_ss5(slot, sbus_offsets_ss5[slot][3]);
    }

    return 0;
}

static int
ob_sbus_init_ss10(unsigned long bus, unsigned long base)
{
    unsigned int slot;
    int notfirst = 0;

    for (slot = 0; slot < SBUS_SLOTS; slot++) {
        if (sbus_offsets_ss10[slot][4] > 0)
            ob_add_sbus_range(sbus_offsets_ss10[slot], notfirst++);
    }
    push_str("ranges");
    fword("property");

    for (slot = 0; slot < SBUS_SLOTS; slot++) {
        if (sbus_offsets_ss10[slot][4] > 0)
            sbus_probe_slot_ss10(slot, sbus_offsets_ss10[slot][3]);
    }

    return 0;
}

int ob_sbus_init(unsigned long bus, unsigned long base, int machine_id)
{
    ob_sbus_node_init(bus, base);

    switch (machine_id) {
    case 0x72:
        return ob_sbus_init_ss10(bus, base);
    case 0x80:
        return ob_sbus_init_ss5(bus, base);
    default:
        return -1;
    }
}
