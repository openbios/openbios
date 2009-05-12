/*
 *   OpenBIOS pci driver
 *
 *   This driver is compliant to the
 *   PCI bus binding to IEEE 1275-1994 Rev 2.1
 *
 *   (C) 2004 Stefan Reinauer <stepan@openbios.org>
 *   (C) 2005 Ed Schouten <ed@fxq.nl>
 *
 *   Some parts from OpenHackWare-0.4, Copyright (c) 2004-2005 Jocelyn Mayer
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/kernel.h"
#include "openbios/pci.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"

#include "openbios/drivers.h"
#include "video_subr.h"
#include "timer.h"
#include "pci.h"
#include "pci_database.h"
#ifdef CONFIG_DRIVER_MACIO
#include "cuda.h"
#include "macio.h"
#endif

#define set_bool_property(ph, name) set_property(ph, name, NULL, 0);

/* DECLARE data structures for the nodes.  */

DECLARE_UNNAMED_NODE( ob_pci_bus_node, INSTALL_OPEN, 2*sizeof(int) );
DECLARE_UNNAMED_NODE( ob_pci_simple_node, INSTALL_OPEN, 2*sizeof(int) );

const pci_arch_t *arch;

#define IS_NOT_RELOCATABLE	0x80000000
#define IS_PREFETCHABLE		0x40000000
#define IS_ALIASED		0x20000000

enum {
	CONFIGURATION_SPACE = 0,
	IO_SPACE = 1,
	MEMORY_SPACE_32 = 2,
	MEMORY_SPACE_64 = 3,
};

static inline void pci_encode_phys_addr(cell *phys, int flags, int space_code,
				 pci_addr dev, uint8_t reg, uint64_t addr)
{

	/* phys.hi */

	phys[0] = flags | (space_code << 24) | dev | reg;

	/* phys.mid */

	phys[1] = addr >> 32;

	/* phys.lo */

	phys[2] = addr;
}

static void
ob_pci_open(int *idx)
{
	int ret=1;
	RET ( -ret );
}

static void
ob_pci_close(int *idx)
{
	selfword("close-deblocker");
}

static void
ob_pci_initialize(int *idx)
{
}

/* ( str len -- phys.lo phys.mid phys.hi ) */

static void
ob_pci_decode_unit(int *idx)
{
	PUSH(0);
	fword("decode-unit-pci-bus");
}

/*  ( phys.lo phy.mid phys.hi -- str len ) */

static void
ob_pci_encode_unit(int *idx)
{
	char buf[28];
	cell hi = POP();
	cell mid = POP();
	cell lo = POP();
	int n, p, t, ss, bus, dev, fn, reg;

	n = hi & IS_NOT_RELOCATABLE;
	p = hi & IS_PREFETCHABLE;
	t = hi & IS_ALIASED;
	ss = (hi >> 24) & 0x03;

	bus = (hi >> 16) & 0xFF;
	dev = (hi >> 11) & 0x1F;
	fn = (hi >> 8) & 0x07;
	reg = hi & 0xFF;

	switch(ss) {
	case CONFIGURATION_SPACE:

		if (fn == 0)	/* DD */
        		snprintf(buf, sizeof(buf), "%x", dev);
		else		/* DD,F */
        		snprintf(buf, sizeof(buf), "%x,%d", dev, fn);
		break;

	case IO_SPACE:

		/* [n]i[t]DD,F,RR,NNNNNNNN */
        	snprintf(buf, sizeof(buf), "%si%s%x,%x,%x,%x",
			 n ? "n" : "",	/* relocatable */
			 t ? "t" : "",	/* aliased */
			 dev, fn, reg, t ? lo & 0x03FF : lo);
		break;

	case MEMORY_SPACE_32:

		/* [n]m[t][p]DD,F,RR,NNNNNNNN */
        	snprintf(buf, sizeof(buf), "%sm%s%s%x,%x,%x,%x",
			 n ? "n" : "",	/* relocatable */
			 t ? "t" : "",	/* aliased */
			 p ? "p" : "",	/* prefetchable */
			 dev, fn, reg, lo );
		break;

	case MEMORY_SPACE_64:

		/* [n]x[p]DD,F,RR,NNNNNNNNNNNNNNNN */
        	snprintf(buf, sizeof(buf), "%sx%s%x,%x,%x,%llx",
			 n ? "n" : "",	/* relocatable */
			 p ? "p" : "",	/* prefetchable */
			 dev, fn, reg, ((uint64_t)mid << 32) | (uint64_t)lo );
		break;
	}
	push_str(buf);
}

NODE_METHODS(ob_pci_bus_node) = {
	{ NULL,			ob_pci_initialize	},
	{ "open",		ob_pci_open		},
	{ "close",		ob_pci_close		},
	{ "decode-unit",	ob_pci_decode_unit	},
	{ "encode-unit",	ob_pci_encode_unit	},
};

NODE_METHODS(ob_pci_simple_node) = {
	{ NULL,			ob_pci_initialize	},
	{ "open",		ob_pci_open		},
	{ "close",		ob_pci_close		},
};

static void pci_set_bus_range(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	cell props[2];

	props[0] = (config->dev >> 16) & 0xFF;
	props[1] = 1;
	set_property(dev, "bus-range", (char *)props, 2 * sizeof(cell));
}

static void pci_host_set_reg(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	cell props[2];

	props[0] = arch->cfg_base;
	props[1] = arch->cfg_len;
	set_property(dev, "reg", (char *)props, 2 * sizeof(cell));
}

static void pci_host_set_ranges(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	cell props[32];
	int ncells;

	ncells = 0;
	if (arch->io_base) {
		pci_encode_phys_addr(props + ncells, 0, IO_SPACE,
				     config->dev, 0, 0);
		ncells += 3;
		props[ncells++] = arch->io_base;
		props[ncells++] = 0x00000000;
		props[ncells++] = arch->io_len;
	}
	if (arch->rbase) {
		pci_encode_phys_addr(props + ncells, 0, MEMORY_SPACE_32,
				     config->dev, 0, 0);
		ncells += 3;
		props[ncells++] = arch->rbase;
		props[ncells++] = 0x00000000;
		props[ncells++] = arch->rlen;
	}
	if (arch->mem_base) {
		pci_encode_phys_addr(props + ncells, 0, MEMORY_SPACE_32,
				     config->dev, 0, arch->mem_base);
		ncells += 3;
		props[ncells++] = arch->mem_base;
		props[ncells++] = 0x00000000;
		props[ncells++] = arch->mem_len;
	}
	set_property(dev, "ranges", (char *)props, ncells * sizeof(cell));
}

int host_config_cb(const pci_config_t *config)
{
	phandle_t aliases;

	aliases = find_dev("/aliases");
	if (aliases)
		set_property(aliases, "pci",
			     config->path, strlen(config->path) + 1);

	pci_host_set_reg(config);
	pci_host_set_ranges(config);
	pci_set_bus_range(config);

	return 0;
}

int bridge_config_cb(const pci_config_t *config)
{
	phandle_t aliases;

	aliases = find_dev("/aliases");
	set_property(aliases, "bridge", config->path, strlen(config->path) + 1);

	pci_set_bus_range(config);

	return 0;
}

int ide_config_cb2 (const pci_config_t *config)
{
	ob_ide_init(config->path,
		    config->assigned[0] & ~0x0000000F,
		    config->assigned[1] & ~0x0000000F,
		    config->assigned[2] & ~0x0000000F,
		    config->assigned[3] & ~0x0000000F);
	return 0;
}

int eth_config_cb (const pci_config_t *config)
{
	phandle_t ph = get_cur_dev();;

	set_property(ph, "network-type", "ethernet", 9);
	set_property(ph, "removable", "network", 8);
	set_property(ph, "category", "net", 4);

        return 0;
}

static inline void pci_decode_pci_addr(pci_addr addr, int *flags,
				       int *space_code, uint32_t *mask)
{
	if (addr & 0x01) {

		*space_code = IO_SPACE;
		*flags = 0;
		*mask = 0x00000001;

	} else if (addr & 0x04) {

		*flags = IS_NOT_RELOCATABLE;
		*space_code = MEMORY_SPACE_64;
		*mask = 0x0000000F;

	} else {

		*space_code = MEMORY_SPACE_32;
		*flags = IS_NOT_RELOCATABLE;
		*mask = 0x0000000F;

	}
}

/*
 * "Designing PCI Cards and Drivers for Power Macintosh Computers", p. 454
 *
 *  "AAPL,address" provides an array of 32-bit logical addresses
 *  Nth entry corresponding to Nth "assigned-address" base address entry.
 */

static void pci_set_AAPL_address(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	cell props[7];
	int ncells, i;

	ncells = 0;
	for (i = 0; i < 6; i++) {
		if (!config->assigned[i] || !config->sizes[i])
			continue;
		props[ncells++] = config->assigned[i] & ~0x0000000F;
	}
	if (ncells)
		set_property(dev, "AAPL,address", (char *)props,
			     ncells * sizeof(cell));
}

static void pci_set_assigned_addresses(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	cell props[32];
	int ncells;
	int i;
	uint32_t mask;
	int flags, space_code;

	ncells = 0;
	for (i = 0; i < 6; i++) {
		if (!config->assigned[i] || !config->sizes[i])
			continue;
		pci_decode_pci_addr(config->assigned[i],
				    &flags, &space_code, &mask);

		pci_encode_phys_addr(props + ncells,
				     flags, space_code, config->dev,
				     PCI_BASE_ADDR_0 + (i * sizeof(uint32_t)),
				     config->assigned[i] & ~mask);
		ncells += 3;

		props[ncells++] = 0x00000000;
		props[ncells++] = config->sizes[i];
	}
	if (ncells)
		set_property(dev, "assigned-addresses", (char *)props,
			     ncells * sizeof(cell));
}

static void pci_set_reg(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	cell props[38];
	int ncells;
	int i;
	uint32_t mask;
	int space_code, flags;

	ncells = 0;
	pci_encode_phys_addr(props + ncells, 0, CONFIGURATION_SPACE,
			     config->dev, 0, 0);
	ncells += 3;

	props[ncells++] = 0x00000000;
	props[ncells++] = 0x00000000;

	for (i = 0; i < 6; i++) {
		if (!config->assigned[i] || !config->sizes[i])
			continue;

		pci_decode_pci_addr(config->regions[i],
				    &flags, &space_code, &mask);

		pci_encode_phys_addr(props + ncells,
				     flags, space_code, config->dev,
				     PCI_BASE_ADDR_0 + (i * sizeof(uint32_t)),
				     config->regions[i] & ~mask);
		ncells += 3;

		/* set size */

		props[ncells++] = 0x00000000;
		props[ncells++] = config->sizes[i];
	}
	set_property(dev, "reg", (char *)props, ncells * sizeof(cell));
}


static void pci_set_ranges(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	cell props[32];
	int ncells;
  	int i;
	uint32_t mask;
	int flags;
	int space_code;

	ncells = 0;
	for (i = 0; i < 6; i++) {
		if (!config->assigned[i] || !config->sizes[i])
			continue;

		/* child address */

		props[ncells++] = 0x00000000;

		/* parent address */

		pci_decode_pci_addr(config->assigned[i],
				    &flags, &space_code, &mask);
		pci_encode_phys_addr(props + ncells, flags, space_code,
				     config->dev, 0x10 + i * 4,
				     config->assigned[i] & ~mask);
		ncells += 3;

		/* size */

		props[ncells++] = config->sizes[i];
  	}
	set_property(dev, "ranges", (char *)props, ncells * sizeof(cell));
}

int macio_heathrow_config_cb (const pci_config_t *config)
{
	pci_set_ranges(config);

#ifdef CONFIG_DRIVER_MACIO
        ob_macio_heathrow_init(config->path, config->assigned[0] & ~0x0000000F);
#endif
	return 0;
}

int macio_keylargo_config_cb (const pci_config_t *config)
{
        pci_set_ranges(config);

#ifdef CONFIG_DRIVER_MACIO
        ob_macio_keylargo_init(config->path, config->assigned[0] & ~0x0000000F);
#endif
        return 0;
}

int vga_config_cb (const pci_config_t *config)
{
	if (config->assigned[0] != 0x00000000)
		vga_vbe_init(config->path, config->assigned[0] & ~0x0000000F,
					   config->sizes[0],
					   config->assigned[1] & ~0x0000000F,
					   config->sizes[1]);
	return 0;
}

int ebus_config_cb(const pci_config_t *config)
{
#ifdef CONFIG_DRIVER_EBUS
#ifdef CONFIG_DRIVER_FLOPPY
    ob_floppy_init(config->path, "fdthree", 0x3f0ULL, 0);
#endif
#ifdef CONFIG_DRIVER_PC_SERIAL
    ob_pc_serial_init(config->path, "su", arch->io_base, 0x3f8ULL, 0);
#endif
#ifdef CONFIG_DRIVER_PC_KBD
    ob_pc_kbd_init(config->path, "kb_ps2", arch->io_base, 0x60ULL, 0);
#endif
#endif
    return 0;
}

static void ob_pci_add_properties(pci_addr addr, const pci_dev_t *pci_dev,
                                  const pci_config_t *config)
{
	phandle_t dev=get_cur_dev();
	int status,id;
	uint16_t vendor_id, device_id;
	uint8_t rev;
	uint32_t class_code;

	vendor_id = pci_config_read16(addr, PCI_VENDOR_ID);
	device_id = pci_config_read16(addr, PCI_DEVICE_ID);
	rev = pci_config_read8(addr, PCI_REVISION_ID);
	class_code = pci_config_read16(addr, PCI_CLASS_DEVICE);

	/* create properties as described in 2.5 */

	set_int_property(dev, "vendor-id", vendor_id);
	set_int_property(dev, "device-id", device_id);
	set_int_property(dev, "revision-id", rev);
	set_int_property(dev, "class-code", class_code << 8);

	if (config->irq_pin) {
		OLDWORLD(set_int_property(dev, "AAPL,interrupts",
					  config->irq_line));
		set_int_property(dev, "interrupts", config->irq_pin);
	}

	set_int_property(dev, "min-grant", pci_config_read8(addr, PCI_MIN_GNT));
	set_int_property(dev, "max-latency", pci_config_read8(addr, PCI_MAX_LAT));

	status=pci_config_read16(addr, PCI_STATUS);

	set_int_property(dev, "devsel-speed",
			(status&PCI_STATUS_DEVSEL_MASK)>>10);

	if(status&PCI_STATUS_FAST_BACK)
		set_bool_property(dev, "fast-back-to-back");
	if(status&PCI_STATUS_66MHZ)
		set_bool_property(dev, "66mhz-capable");
	if(status&PCI_STATUS_UDF)
		set_bool_property(dev, "udf-supported");

	id=pci_config_read16(addr, PCI_SUBSYSTEM_VENDOR_ID);
	if(id)
		set_int_property(dev, "subsystem-vendor-id", id);
	id=pci_config_read16(addr, PCI_SUBSYSTEM_ID);
	if(id)
		set_int_property(dev, "subsystem-id", id);

	set_int_property(dev, "cache-line-size",
			pci_config_read16(addr, PCI_CACHE_LINE_SIZE));

	if (pci_dev) {
		if (pci_dev->type) {
			push_str(pci_dev->type);
			fword("encode-string");
			push_str("device_type");
			fword("property");
		}
		if (pci_dev->model) {
			push_str(pci_dev->model);
			fword("encode-string");
			push_str("model");
			fword("property");
		}
		if (pci_dev->compat)
			set_property(dev, "compatible",
				     pci_dev->compat, pci_compat_len(pci_dev));

		if (pci_dev->acells)
			set_int_property(dev, "#address-cells",
					      pci_dev->acells);
		if (pci_dev->scells)
			set_int_property(dev, "#size-cells",
					       pci_dev->scells);
		if (pci_dev->icells)
			set_int_property(dev, "#interrupt-cells",
					      pci_dev->icells);
	}

	pci_set_reg(config);
	pci_set_assigned_addresses(config);
	OLDWORLD(pci_set_AAPL_address(config));

#ifdef CONFIG_DEBUG_PCI
	printk("\n");
#endif

	if (pci_dev && pci_dev->config_cb)
		pci_dev->config_cb(config);
}

#ifdef CONFIG_XBOX
static char pci_xbox_blacklisted (int bus, int devnum, int fn)
{
	/*
	 * The Xbox MCPX chipset is a derivative of the nForce 1
	 * chipset. It almost has the same bus layout; some devices
	 * cannot be used, because they have been removed.
	 */

	/*
	 * Devices 00:00.1 and 00:00.2 used to be memory controllers on
	 * the nForce chipset, but on the Xbox, using them will lockup
	 * the chipset.
	 */
	if ((bus == 0) && (devnum == 0) && ((fn == 1) || (fn == 2)))
		return 1;

	/*
	 * Bus 1 only contains a VGA controller at 01:00.0. When you try
	 * to probe beyond that device, you only get garbage, which
	 * could cause lockups.
	 */
	if ((bus == 1) && ((devnum != 0) || (fn != 0)))
		return 1;

	/*
	 * Bus 2 used to contain the AGP controller, but the Xbox MCPX
	 * doesn't have one. Probing it can cause lockups.
	 */
	if (bus >= 2)
		return 1;

	/*
	 * The device is not blacklisted.
	 */
	return 0;
}
#endif

static void
ob_pci_configure(pci_addr addr, pci_config_t *config, unsigned long *mem_base,
                 unsigned long *io_base)

{
	uint32_t smask, omask, amask, size, reloc, min_align;
        unsigned long base;
	pci_addr config_addr;
	int reg;
	uint8_t irq_pin, irq_line;

	irq_pin =  pci_config_read8(addr, PCI_INTERRUPT_PIN);
	if (irq_pin) {
		config->irq_pin = irq_pin;
		irq_pin = (((config->dev >> 11) & 0x1F) + irq_pin - 1) & 3;
		irq_line = arch->irqs[irq_pin];
		pci_config_write8(addr, PCI_INTERRUPT_LINE, irq_line);
		config->irq_line = irq_line;
	} else
		config->irq_line = -1;

	omask = 0x00000000;
	for (reg = 0; reg < 7; reg++) {

		config->assigned[reg] = 0x00000000;
		config->sizes[reg] = 0x00000000;

		if ((omask & 0x0000000f) == 0x4) {
			/* 64 bits memory mapping */
			continue;
		}

		if (reg == 6)
			config_addr = PCI_ROM_ADDRESS;
		else
			config_addr = PCI_BASE_ADDR_0 + reg * 4;

		config->regions[reg] = pci_config_read32(addr, config_addr);

		/* get region size */

		pci_config_write32(addr, config_addr, 0xffffffff);
		smask = pci_config_read32(addr, config_addr);
		if (smask == 0x00000000 || smask == 0xffffffff)
			continue;

		if (smask & 0x00000001 && reg != 6) {
			/* I/O space */
			base = *io_base;
			min_align = 1 << 7;
			amask = 0x00000001;
			pci_config_write16(addr, PCI_COMMAND,
					   pci_config_read16(addr,
					   		     PCI_COMMAND) |
							     PCI_COMMAND_IO);
		} else {
			/* Memory Space */
			base = *mem_base;
			min_align = 1 << 16;
			amask = 0x0000000F;
			if (reg == 6) {
				smask |= 1; /* ROM */
			}
			pci_config_write16(addr, PCI_COMMAND,
					   pci_config_read16(addr,
					   		    PCI_COMMAND) |
							    PCI_COMMAND_MEMORY);
		}
		omask = smask & amask;
		smask &= ~amask;
		size = (~smask) + 1;
		config->sizes[reg] = size;
		reloc = base;
		if (size < min_align)
			size = min_align;
		reloc = (reloc + size -1) & ~(size - 1);
		if (*io_base == base) {
			*io_base = reloc + size;
			reloc -= arch->io_base;
		} else {
			*mem_base = reloc + size;
		}
		pci_config_write32(addr, config_addr, reloc | omask);
		config->assigned[reg] = reloc | omask;
	}
}

static void ob_scan_pci_bus(int bus, unsigned long *mem_base,
                            unsigned long *io_base, char **path)
{
	int devnum, fn, is_multi, vid, did;
	unsigned int htype;
	pci_addr addr;
	pci_config_t config;
        const pci_dev_t *pci_dev;
	uint32_t ccode;
	uint8_t class, subclass, iface, rev;

	activate_device("/");
	for (devnum = 0; devnum < 32; devnum++) {
		is_multi = 0;
		for (fn = 0; fn==0 || (is_multi && fn<8); fn++) {
#ifdef CONFIG_XBOX
			if (pci_xbox_blacklisted (bus, devnum, fn))
				continue;
#endif
			addr = PCI_ADDR(bus, devnum, fn);
			vid = pci_config_read16(addr, PCI_VENDOR_ID);
			did = pci_config_read16(addr, PCI_DEVICE_ID);

			if (vid==0xffff || vid==0)
				continue;

			ccode = pci_config_read16(addr, PCI_CLASS_DEVICE);
			class = ccode >> 8;
			subclass = ccode;
			iface = pci_config_read8(addr, PCI_CLASS_PROG);
			rev = pci_config_read8(addr, PCI_REVISION_ID);

			pci_dev = pci_find_device(class, subclass, iface,
						  vid, did);

#ifdef CONFIG_DEBUG_PCI
			printk("%x:%x.%x - %x:%x - ", bus, devnum, fn,
					vid, did);
#endif
			htype = pci_config_read8(addr, PCI_HEADER_TYPE);
			if (fn == 0)
				is_multi = htype & 0x80;

			if (pci_dev == NULL || pci_dev->name == NULL)
                            snprintf(config.path, sizeof(config.path),
				     "%s/pci%x,%x", *path, vid, did);
			else
                            snprintf(config.path, sizeof(config.path),
				     "%s/%s", *path, pci_dev->name);
#ifdef CONFIG_DEBUG_PCI
			printk("%s - ", config.path);
#endif
			config.dev = addr & 0x00FFFFFF;

                        if (class == PCI_BASE_CLASS_BRIDGE &&
                            (subclass == PCI_SUBCLASS_BRIDGE_HOST ||
                             subclass == PCI_SUBCLASS_BRIDGE_PCI))
                            REGISTER_NAMED_NODE(ob_pci_bus_node, config.path);
                        else
                            REGISTER_NAMED_NODE(ob_pci_simple_node, config.path);

			activate_device(config.path);

                        ob_pci_configure(addr, &config, mem_base, io_base);
			ob_pci_add_properties(addr, pci_dev, &config);

                        if (class == PCI_BASE_CLASS_BRIDGE &&
                            (subclass == PCI_SUBCLASS_BRIDGE_HOST ||
                             subclass == PCI_SUBCLASS_BRIDGE_PCI)) {
				/* host or bridge */
				free(*path);
				*path = strdup(config.path);
			}

		}
	}
	device_end();
}

int ob_pci_init(void)
{
        int bus;
        unsigned long mem_base, io_base;
	char *path;

#ifdef CONFIG_DEBUG_PCI
	printk("Initializing PCI devices...\n");
#endif

	/* brute force bus scan */

	/* Find all PCI bridges */

	mem_base = arch->mem_base;
        /* I/O ports under 0x400 are used by devices mapped at fixed
           location. */
        io_base = arch->io_base + 0x400;
	path = strdup("");
	for (bus = 0; bus<0x100; bus++) {
		ob_scan_pci_bus(bus, &mem_base, &io_base, &path);
	}
	free(path);
	return 0;
}
