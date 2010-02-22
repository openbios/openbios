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

static inline void pci_encode_phys_addr(u32 *phys, int flags, int space_code,
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
}

static void
ob_pci_initialize(int *idx)
{
}

/* ( str len -- phys.lo phys.mid phys.hi ) */

static void
ob_pci_decode_unit(int *idx)
{
	ucell hi, mid, lo;
	const char *arg = pop_fstr_copy();
	int dev, fn, reg, ss, n, p, t;
	int bus = 0;		/* no information */
	char *ptr;

	dev = 0;
	fn = 0;
	reg = 0;
	ss = 0;
	n = 0;
	p = 0;
	t = 0;

	ptr = (char*)arg;
	if (*ptr == 'n') {
		n = IS_NOT_RELOCATABLE;
		ptr++;
	}
	if (*ptr == 'i') {
		ss = IO_SPACE;
		ptr++;
		if (*ptr == 't') {
			t = IS_ALIASED;
			ptr++;
		}

		/* DD,F,RR,NNNNNNNN */

		dev = strtol(ptr, &ptr, 16);
		ptr++;
		fn = strtol(ptr, &ptr, 16);
		ptr++;
		reg = strtol(ptr, &ptr, 16);
		ptr++;
		lo = strtol(ptr, &ptr, 16);
		mid = 0;

	} else if (*ptr == 'm') {
		ss = MEMORY_SPACE_32;
		ptr++;
		if (*ptr == 't') {
			t = IS_ALIASED;
			ptr++;
		}
		if (*ptr == 'p') {
			p = IS_PREFETCHABLE;
			ptr++;
		}

		/* DD,F,RR,NNNNNNNN */

		dev = strtol(ptr, &ptr, 16);
		ptr++;
		fn = strtol(ptr, &ptr, 16);
		ptr++;
		reg = strtol(ptr, &ptr, 16);
		ptr++;
		lo = strtol(ptr, &ptr, 16);
		mid = 0;

	} else if (*ptr == 'x') {
		unsigned long long addr64;
		ss = MEMORY_SPACE_64;
		ptr++;
		if (*ptr == 'p') {
			p = IS_PREFETCHABLE;
			ptr++;
		}

		/* DD,F,RR,NNNNNNNNNNNNNNNN */

		dev = strtol(ptr, &ptr, 16);
		ptr++;
		fn = strtol(ptr, &ptr, 16);
		ptr++;
		reg = strtol(ptr, &ptr, 16);
		ptr++;
		addr64 = strtoll(ptr, &ptr, 16);
		lo = (ucell)addr64;
		mid = addr64 >> 32;

	} else {
		ss = CONFIGURATION_SPACE;
		/* "DD" or "DD,FF" */
		dev = strtol(ptr, &ptr, 16);
		if (*ptr == ',') {
			ptr++;
			fn = strtol(ptr, NULL, 16);
		}
		lo = 0;
		mid = 0;
	}
	free((char*)arg);

	hi = n | p | t | (ss << 24) | (bus << 16) | (dev << 11) | (fn << 8) | reg;

	PUSH(lo);
	PUSH(mid);
	PUSH(hi);
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
        		snprintf(buf, sizeof(buf), "%x,%x", dev, fn);
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
	u32 props[2];

	props[0] = (config->dev >> 16) & 0xFF;
	props[1] = 1;
	set_property(dev, "bus-range", (char *)props, 2 * sizeof(props[0]));
}

static void pci_host_set_interrupt_map(const pci_config_t *config)
{
/* XXX We currently have a hook in the MPIC init code to fill in its handle.
 *     If you want to have interrupt maps for your PCI host bus, add your
 *     architecture to the #if and make your bridge detect code fill in its
 *     handle too.
 *
 *     It would be great if someone clever could come up with a more universal
 *     mechanism here.
 */
#if defined(CONFIG_PPC)
	phandle_t dev = get_cur_dev();
	u32 props[7 * 4];
	int i;

#if defined(CONFIG_PPC)
	/* Oldworld macs do interrupt maps differently */
	if(!is_newworld())
		return;
#endif

	for (i = 0; i < (7*4); i+=7) {
		props[i+PCI_INT_MAP_PCI0] = 0;
		props[i+PCI_INT_MAP_PCI1] = 0;
		props[i+PCI_INT_MAP_PCI2] = 0;
		props[i+PCI_INT_MAP_PCI_INT] = (i / 7) + 1; // starts at PINA=1
		props[i+PCI_INT_MAP_PIC_HANDLE] = 0; // gets patched in later
		props[i+PCI_INT_MAP_PIC_INT] = arch->irqs[i / 7];
		props[i+PCI_INT_MAP_PIC_POL] = 3;
	}
	set_property(dev, "interrupt-map", (char *)props, 7 * 4 * sizeof(props[0]));

	props[PCI_INT_MAP_PCI0] = 0;
	props[PCI_INT_MAP_PCI1] = 0;
	props[PCI_INT_MAP_PCI2] = 0;
	props[PCI_INT_MAP_PCI_INT] = 0x7;

	set_property(dev, "interrupt-map-mask", (char *)props, 4 * sizeof(props[0]));
#endif
}

static void pci_host_set_reg(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	u32 props[2];

	props[0] = arch->cfg_base;
	props[1] = arch->cfg_len;
	set_property(dev, "reg", (char *)props, 2 * sizeof(props[0]));
}

static void pci_host_set_ranges(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	u32 props[32];
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
	set_property(dev, "ranges", (char *)props, ncells * sizeof(props[0]));
}

static unsigned long pci_bus_addr_to_host_addr(uint32_t ba)
{
#ifdef CONFIG_SPARC64
    return arch->cfg_data + (unsigned long)ba;
#else
    return (unsigned long)ba;
#endif
}

int host_config_cb(const pci_config_t *config)
{
	phandle_t aliases;

	aliases = find_dev("/aliases");
	if (aliases)
		set_property(aliases, "pci",
			     config->path, strlen(config->path) + 1);

	//XXX this overrides "reg" property
	pci_host_set_reg(config);
	pci_host_set_ranges(config);
	pci_set_bus_range(config);
	pci_host_set_interrupt_map(config);

	return 0;
}

int sabre_config_cb(const pci_config_t *config)
{
        phandle_t dev = get_cur_dev();
        uint32_t props[28];

        props[0] = 0x00000000;
        props[1] = 0x00000003;
        set_property(dev, "bus-range", (char *)props, 2 * sizeof(props[0]));
        props[0] = 0x000001fe;
        props[1] = 0x00000000;
        props[2] = 0x00000000;
        props[3] = 0x00010000;
        props[4] = 0x000001fe;
        props[5] = 0x01000000;
        props[6] = 0x00000000;
        props[7] = 0x00000100;
        set_property(dev, "reg", (char *)props, 8 * sizeof(props[0]));
        props[0] = 0x00000000;
        props[1] = 0x00000000;
        props[2] = 0x00000000;
        props[3] = 0x000001fe;
        props[4] = 0x01000000;
        props[5] = 0x00000000;
        props[6] = 0x01000000;
        props[7] = 0x01000000;
        props[8] = 0x00000000;
        props[9] = 0x00000000;
        props[10] = 0x000001fe;
        props[11] = 0x02000000;
        props[12] = 0x00000000;
        props[13] = 0x01000000;
        props[14] = 0x02000000;
        props[15] = 0x00000000;
        props[16] = 0x00000000;
        props[17] = 0x000001ff;
        props[18] = 0x00000000;
        props[19] = 0x00000001;
        props[20] = 0x00000000;
        props[21] = 0x03000000;
        props[22] = 0x00000000;
        props[23] = 0x00000000;
        props[24] = 0x000001ff;
        props[25] = 0x00000000;
        props[26] = 0x00000001;
        props[27] = 0x00000000;
        set_property(dev, "ranges", (char *)props, 28 * sizeof(props[0]));
        props[0] = 0xc0000000;
        props[1] = 0x20000000;
        set_property(dev, "virtual-dma", (char *)props, 2 * sizeof(props[0]));
        props[0] = 1;
        set_property(dev, "#virtual-dma-size-cells", (char *)props,
                     sizeof(props[0]));
        set_property(dev, "#virtual-dma-addr-cells", (char *)props,
                     sizeof(props[0]));
        props[0] = 0x000007f0;
        props[1] = 0x000007ee;
        props[2] = 0x000007ef;
        props[3] = 0x000007e5;
        set_property(dev, "interrupts", (char *)props, 4 * sizeof(props[0]));
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
	phandle_t ph = get_cur_dev();

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

static void pci_set_assigned_addresses(const pci_config_t *config, int num_bars)
{
	phandle_t dev = get_cur_dev();
	u32 props[32];
	int ncells;
	int i;
	uint32_t mask;
	int flags, space_code;

	ncells = 0;
	for (i = 0; i < num_bars; i++) {
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
			     ncells * sizeof(props[0]));
}

static void pci_set_reg(const pci_config_t *config, int num_bars)
{
	phandle_t dev = get_cur_dev();
	u32 props[38];
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

	for (i = 0; i < num_bars; i++) {
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
	set_property(dev, "reg", (char *)props, ncells * sizeof(props[0]));
}


static void pci_set_ranges(const pci_config_t *config)
{
	phandle_t dev = get_cur_dev();
	u32 props[32];
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
	set_property(dev, "ranges", (char *)props, ncells * sizeof(props[0]));
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
            vga_vbe_init(config->path,
                         pci_bus_addr_to_host_addr(config->assigned[0] & ~0x0000000F),
                         config->sizes[0],
                         pci_bus_addr_to_host_addr(config->assigned[1] & ~0x0000000F),
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
                                  const pci_config_t *config, int num_bars)
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

	pci_set_reg(config, num_bars);
	pci_set_assigned_addresses(config, num_bars);
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

static void ob_pci_configure_bar(pci_addr addr, pci_config_t *config,
                                 int reg, int config_addr,
                                 uint32_t *p_omask,
                                 unsigned long *mem_base,
                                 unsigned long *io_base)
{
        uint32_t smask, amask, size, reloc, min_align;
        unsigned long base;

        config->assigned[reg] = 0x00000000;
        config->sizes[reg] = 0x00000000;

        if ((*p_omask & 0x0000000f) == 0x4) {
                /* 64 bits memory mapping */
                return;
        }

        config->regions[reg] = pci_config_read32(addr, config_addr);

        /* get region size */

        pci_config_write32(addr, config_addr, 0xffffffff);
        smask = pci_config_read32(addr, config_addr);
        if (smask == 0x00000000 || smask == 0xffffffff)
                return;

        if (smask & 0x00000001 && reg != 6) {
                /* I/O space */
                base = *io_base;
                min_align = 1 << 7;
                amask = 0x00000001;
        } else {
                /* Memory Space */
                base = *mem_base;
                min_align = 1 << 16;
                amask = 0x0000000F;
                if (reg == 6) {
                        smask |= 1; /* ROM */
                }
        }
        *p_omask = smask & amask;
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
        pci_config_write32(addr, config_addr, reloc | *p_omask);
        config->assigned[reg] = reloc | *p_omask;
}

static void ob_pci_configure_irq(pci_addr addr, pci_config_t *config)
{
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
}

static void
ob_pci_configure(pci_addr addr, pci_config_t *config, int num_regs, int rom_bar,
                 unsigned long *mem_base, unsigned long *io_base)

{
        uint32_t omask;
        uint16_t cmd;
        int reg;
        pci_addr config_addr;

        ob_pci_configure_irq(addr, config);

        omask = 0x00000000;
        for (reg = 0; reg < num_regs; ++reg) {
                config_addr = PCI_BASE_ADDR_0 + reg * 4;

                ob_pci_configure_bar(addr, config, reg, config_addr,
                                     &omask, mem_base,
                                     io_base);
        }

        if (rom_bar) {
                config_addr = rom_bar;
                ob_pci_configure_bar(addr, config, reg, config_addr,
                                     &omask, mem_base, io_base);
        }
        cmd = pci_config_read16(addr, PCI_COMMAND);
        cmd |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY;
        pci_config_write16(addr, PCI_COMMAND, cmd);
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
	int num_bars, rom_bar;

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

			if (htype & PCI_HEADER_TYPE_BRIDGE) {
				num_bars = 2;
				rom_bar  = PCI_ROM_ADDRESS1;
			} else {
				num_bars = 6;
				rom_bar  = PCI_ROM_ADDRESS;
			}

			ob_pci_configure(addr, &config, num_bars, rom_bar,
                                         mem_base, io_base);
			ob_pci_add_properties(addr, pci_dev, &config, num_bars);

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
