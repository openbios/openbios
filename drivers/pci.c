/*
 *   OpenBIOS pci driver
 *   
 *   This driver is compliant to the 
 *   PCI bus binding to IEEE 1275-1994 Rev 2.1
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
#include "timer.h"
#include "pci.h"

#define REGISTER_NAMED_NODE( name, path )   do { \
	     bind_new_node( name##_flags_, name##_size_, \
			path, name##_m, sizeof(name##_m)/sizeof(method_t)); \
	} while(0)

#define set_bool_property(ph, name) set_property(ph, name, NULL, 0);

/* DECLARE data structures for the nodes.  */
DECLARE_UNNAMED_NODE( ob_pci_node, INSTALL_OPEN, 2*sizeof(int) );

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


NODE_METHODS(ob_pci_node) = {
	{ NULL,			ob_pci_initialize	},
	{ "open",		ob_pci_open		},
	{ "close",		ob_pci_close		},
};

static int ob_pci_add_properties(pci_addr addr)
{
	phandle_t dev=get_cur_dev();
	int status,id;

	/* create properties as described in 2.5 */
	
	set_int_property(dev, "vendor-id", pci_config_read16(addr, PCI_VENDOR_ID));
	set_int_property(dev, "device-id", pci_config_read16(addr, PCI_DEVICE_ID));
	set_int_property(dev, "revision-id", pci_config_read8(addr, PCI_REVISION_ID));
	
	set_int_property(dev, "interrupts", 
			pci_config_read8(addr, PCI_INTERRUPT_LINE));
	
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
	
}


static int ob_pci_add_reg(pci_addr addr)
{
	PUSH(0);
	PUSH(0);
	PUSH(addr&(~0x80000000u));
	fword("pci-addr-encode");
	PUSH(0);
	PUSH(0);
	fword("pci-len-encode");
	fword("encode+");
	push_str("reg");
	fword("property");
}

static int ob_pci_add_class(pci_addr addr)
{
	unsigned int class;
	char *strc;

	class=pci_config_read16(addr, PCI_CLASS_DEVICE);
	switch (class) {
	case 0x001: strc="display"; break;
	case 0x100: strc="scsi"; break;
	case 0x101: strc="ide"; break;
	case 0x102: strc="fdc"; break;
	case 0x103: strc="ipi"; break;
	case 0x104: strc="raid"; break;
	case 0x200: strc="ethernet"; break;
	case 0x201: strc="token-ring"; break;
	case 0x202: strc="fddi"; break;
	case 0x203: strc="atm"; break;
	case 0x400: strc="video"; break;
	case 0x401: strc="sound"; break;
	case 0x500: strc="memory"; break;
	case 0x501: strc="flash"; break;
	case 0x600: strc="host"; break;
	case 0x601: strc="isa"; break;
	case 0x602: strc="eisa"; break;
	case 0x603: strc="mca"; break;
	case 0x604: strc="pci"; break;
	case 0x605: strc="pcmcia"; break;
	case 0x606: strc="nubus"; break;
	case 0x607: strc="cardbus"; break;
	case 0x680: strc="pmu"; break; /* not in pci binding */
	case 0x700: strc="serial"; break;
	case 0x701: strc="parallel"; break;
	case 0x703: strc="modem"; break; /* not in pci binding */
	case 0x800: strc="interrupt-controller"; break;
	case 0x801: strc="dma-controller"; break;
	case 0x802: strc="timer"; break;
	case 0x803: strc="rtc"; break;
	case 0x900: strc="keyboard"; break;
	case 0x901: strc="pen"; break;
	case 0x902: strc="mouse"; break;
	case 0xC00: strc="firewire"; break;
	case 0xC01: strc="access-bus"; break;
	case 0xC02: strc="ssa"; break;
	case 0xC03: strc="usb"; break;
	case 0xC04: strc="fibre-channel"; break;
	default: 
		switch (class>>8) {
		case 0x3: strc="display"; break;
		case 0xA: strc="dock"; break;
		case 0xB: strc="cpu"; break;
		default:strc="unknown"; break;
		}
	}
	if ((class>>8) == 0x03) {
		push_str(strc);
		fword("encode-string");
		push_str("device_type");
		fword("property");
	}
	push_str(strc);
	fword("encode-string");
	push_str("class");
	fword("property");

	
#ifdef CONFIG_DEBUG_PCI
	printk("%s\n", strc);
#endif
}

static void
pci_enable_rom(pci_addr dev)
{
	u32 rom_addr;

	rom_addr=pci_config_read32(dev, PCI_ROM_ADDRESS);
	rom_addr |= PCI_ROM_ADDRESS_ENABLE;
	pci_config_write32(dev, PCI_ROM_ADDRESS, rom_addr);
}

static void
pci_disable_rom(pci_addr dev)
{
	u32 rom_addr;
	
	rom_addr=pci_config_read32(dev, PCI_ROM_ADDRESS);
	rom_addr &= ~PCI_ROM_ADDRESS_ENABLE;
	pci_config_write32(dev, PCI_ROM_ADDRESS, rom_addr);
}
	

static void ob_pci_scan_rom(pci_addr addr)
{
	unsigned long rom_addr=pci_config_read32(addr,PCI_ROM_ADDRESS);
	unsigned char *walk;
	
	rom_addr &= PCI_ROM_ADDRESS_MASK;
	if(rom_addr) {
		printk("  ROM found at 0x%lx\n",rom_addr);
		pci_enable_rom(addr);
		walk=phys_to_virt((unsigned char *)rom_addr);
		if (walk[0]!=0x55 || walk[1]!=0xaa) {
			printk("no pci rom\n");
			goto rom_err;
		}
		
rom_err:
		pci_disable_rom(addr);
	}
	
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

static void ob_scan_pci_bus(int bus, char *path)
{
	int devnum, fn, is_multi, vid, did;
	unsigned int htype;
	pci_addr addr;
	char * nodetemp = "%s/pci%x,%x";
	char nodebuff[32];
	phandle_t dnode, dbus;

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
#ifdef CONFIG_DEBUG_PCI
			printk("%x:%x.%x - %x:%x - ", bus, devnum, fn, 
					vid, did);
#endif
			htype = pci_config_read8(addr, PCI_HEADER_TYPE);
			if (fn == 0)
				is_multi = htype & 0x80;
	    
			activate_device(path);

			dbus=get_cur_dev();
			sprintf(nodebuff, nodetemp, path, vid, did);
#ifdef CONFIG_DEBUG_PCI
			printk("%s - ", nodebuff);
#endif
			REGISTER_NAMED_NODE(ob_pci_node, nodebuff);
			dnode=find_dev(nodebuff);
			activate_dev( dnode );

			ob_pci_add_properties(addr);
			ob_pci_add_reg(addr);
			ob_pci_add_class(addr);
			ob_pci_scan_rom(addr);
			device_end();
			activate_dev( dbus );
		}
	}
}


int ob_pci_init(void)
{
	char *path="/pci";
	int bus;

	printk("Initializing PCI devices...\n");
	
	/* brute force bus scan */
	for (bus=0; bus<0x100; bus++) {
		ob_scan_pci_bus(bus, path);
	}
	return 0;
}
