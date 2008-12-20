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
#include "openbios/pci.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"

#include "openbios/drivers.h"
#include "timer.h"
#include "pci.h"

#define set_bool_property(ph, name) set_property(ph, name, NULL, 0);

/* DECLARE data structures for the nodes.  */
DECLARE_UNNAMED_NODE( ob_pci_node, INSTALL_OPEN, 2*sizeof(int) );

pci_arch_t *arch;

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

/* PCI devices database */
typedef struct pci_class_t pci_class_t;
typedef struct pci_subclass_t pci_subclass_t;
typedef struct pci_iface_t pci_iface_t;
typedef struct pci_dev_t pci_dev_t;

typedef struct pci_config_t pci_config_t;

struct pci_config_t {
	char path[64];
	uint32_t regions[7];
	uint32_t sizes[7];
};

struct pci_iface_t {
    uint8_t iface;
    const char *name;
    const char *type;
    const pci_dev_t *devices;
    int (*config_cb)(const pci_config_t *config);
    const void *private;
};

struct pci_subclass_t {
    uint8_t subclass;
    const char *name;
    const char *type;
    const pci_dev_t *devices;
    const pci_iface_t *iface;
    int (*config_cb)(const pci_config_t *config);
    const void *private;
};

struct pci_class_t {
    const char *name;
    const char *type;
    const pci_subclass_t *subc;
};

struct pci_dev_t {
    uint16_t vendor;
    uint16_t product;
    const char *type;
    const char *name;
    const char *model;
    const char *compat;
    int acells;
    int scells;
    int icells;
    int (*config_cb)(const pci_config_t *config);
    const void *private;
};

/* Current machine description */

typedef struct pci_bridge_t pci_bridge_t;

/* Low level access helpers */
struct pci_ops_t {
    uint8_t (*config_readb)(pci_bridge_t *bridge,
                            uint8_t bus, uint8_t devfn, uint8_t offset);
    void (*config_writeb)(pci_bridge_t *bridge,
                          uint8_t bus, uint8_t devfn,
                          uint8_t offset, uint8_t val);
    uint16_t (*config_readw)(pci_bridge_t *bridge,
                             uint8_t bus, uint8_t devfn, uint8_t offset);
    void (*config_writew)(pci_bridge_t *bridge,
                          uint8_t bus, uint8_t devfn,
                          uint8_t offset, uint16_t val);
    uint32_t (*config_readl)(pci_bridge_t *bridge,
                             uint8_t bus, uint8_t devfn, uint8_t offset);
    void (*config_writel)(pci_bridge_t *bridge,
                          uint8_t bus, uint8_t devfn,
                          uint8_t offset, uint32_t val);
};

static const pci_subclass_t undef_subclass[] = {
#if 0
    {
        0x00, "misc undefined", NULL, NULL, NULL,
        NULL, NULL,
    },
#endif
    {
        0xFF, NULL, NULL, NULL, NULL,
        NULL, NULL,
    },
};

static int ide_config_cb2 (const pci_config_t *config)
{
	ob_ide_init(config->path,
		    config->regions[0] & ~0x0000000F,
		    config->regions[1] & ~0x0000000F,
		    config->regions[2] & ~0x0000000F,
		    config->regions[3] & ~0x0000000F);
	return 0;
}

static const pci_dev_t ide_devices[] = {
    {
        0x1095, 0x0646, /* CMD646 IDE controller */
        "pci-ide", "pci-ata", NULL, NULL,
        0, 0, 0,
        ide_config_cb2, NULL,
    },
    {
        0xFFFF, 0xFFFF,
        NULL, NULL, NULL, NULL,
        -1, -1, -1,
        NULL, NULL,
    },
};

static const pci_subclass_t mass_subclass[] = {
    {
        0x00, "SCSI bus controller",        NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x01, "IDE controller",             "ide", ide_devices, NULL,
        NULL, NULL,
    },
    {
        0x02, "Floppy disk controller",     NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x03, "IPI bus controller",         NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x04, "RAID controller",            NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x05, "ATA controller",             "ata", NULL, NULL,
        NULL, NULL,
    },
    {
        0x80, "misc mass-storage controller", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_dev_t eth_devices[] = {
    { 0x10EC, 0x8029,
      NULL, "NE2000",   "NE2000 PCI",  NULL,
      0, 0, 0,
      NULL, "ethernet",
    },
    {
        0xFFFF, 0xFFFF,
        NULL, NULL, NULL, NULL,
        -1, -1, -1,
        NULL, NULL,
    },
};

static void eth_config_cb (const pci_config_t *config)
{
	phandle_t ph;
	cell props[12];
	int i;

	ph = find_dev(config->path);

	set_property(ph, "network-type", "ethernet", 9);
	set_property(ph, "removable", "network", 8);
	set_property(ph, "category", "net", 4);

	for (i = 0; i < 7; i++)
	{
		props[i*2] = config->regions[i];
		props[i*2 + 1] = config->sizes[i];
	}
	set_property(ph, "reg", props, i * 2 * sizeof(cell));
}

static const pci_subclass_t net_subclass[] = {
    {
        0x00, "ethernet controller",       NULL, eth_devices, NULL,
        eth_config_cb, "ethernet",
    },
    {
        0x01, "token ring controller",      NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x02, "FDDI controller",            NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x03, "ATM controller",             NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x04, "ISDN controller",            NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x05, "WordFip controller",         NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x06, "PICMG 2.14 controller",      NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x80, "misc network controller",    NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_dev_t vga_devices[] = {
    {
        0x1002, 0x5046,
        NULL, "ATY",      "ATY Rage128", "VGA",
        0, 0, 0,
        NULL, NULL,
    },
    {
        0x1234, 0x1111,
        NULL, "QEMU,VGA", "Qemu VGA",    "VGA",
        0, 0, 0,
        NULL, NULL,
    },
    {
        0xFFFF, 0xFFFF,
        NULL, NULL, NULL, NULL,
        -1, -1, -1,
        NULL, NULL,
    },
};

/* VGA configuration */

static int vga_config_cb (const pci_config_t *config)
{
	if (config->regions[0] != 0x00000000)
		vga_vbe_init(config->path, config->regions[0], config->sizes[0],
			     config->regions[1], config->sizes[1]);
	return 0;
}

static const struct pci_iface_t vga_iface[] = {
    {
        0x00, "VGA controller", NULL,
        vga_devices, &vga_config_cb, NULL,
    },
    {
        0x01, "8514 compatible controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_subclass_t displ_subclass[] = {
    {
        0x00, "display controller",         NULL,  NULL, vga_iface,
        NULL, NULL,
    },
    {
        0x01, "XGA display controller",     NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x02, "3D display controller",      NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x80, "misc display controller",    NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_subclass_t media_subclass[] = {
    {
        0x00, "video device",              NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x01, "audio device",              NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x02, "computer telephony device", NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x80, "misc multimedia device",    NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_subclass_t mem_subclass[] = {
    {
        0x00, "RAM controller",             NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x01, "flash controller",           NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_dev_t grackle_fake_bridge = {
    0xFFFF, 0xFFFF,
    "pci", "pci-bridge", "DEC,21154", "DEC,21154.pci-bridge",
    -1, -1, -1,
    NULL, NULL,
};

static const pci_dev_t uninorth_agp_fake_bridge = {
    0xFFFF, 0xFFFF,
    "uni-north-agp", "uni-north-agp", NULL, "uni-north-agp",
    -1, -1, -1,
    NULL, NULL,
};

static const pci_dev_t uninorth_fake_bridge = {
    0xFFFF, 0xFFFF,
    "uni-north", "uni-north", NULL, "uni-north",
    -1, -1, -1,
    NULL, NULL,
};


static const pci_dev_t hbrg_devices[] = {
    {
        0x106B, 0x0020, NULL,
        "pci", "AAPL,UniNorth", "uni-north",
        3, 2, 1,
        NULL, &uninorth_agp_fake_bridge,
    },
    {
        0x106B, 0x001F, NULL,
        "pci", "AAPL,UniNorth", "uni-north",
        3, 2, 1,
        NULL, &uninorth_fake_bridge,
    },
    {
        0x106B, 0x001E, NULL,
        "pci", "AAPL,UniNorth", "uni-north",
        3, 2, 1,
        NULL, &uninorth_fake_bridge,
    },
    {
        0x1057, 0x0002, "pci",
        "pci", "MOT,MPC106", "grackle",
        3, 2, 1,
        NULL, &grackle_fake_bridge,
    },
    {
        0x1057, 0x4801, NULL,
        "pci-bridge", "PREP Host PCI Bridge - Motorola Raven", NULL,
        3, 2, 1,
        NULL, NULL,
    },
    {
        0xFFFF, 0xFFFF,
        NULL, NULL, NULL, NULL,
        -1, -1, -1,
        NULL, NULL,
    },
};

static const pci_dev_t PCIbrg_devices[] = {
    {
        0x1011, 0x0026, NULL,
        "pci-bridge", NULL, NULL,
        3, 2, 1,
        NULL, NULL,
    },
    {
        0xFFFF, 0xFFFF,
        NULL, NULL, NULL, NULL,
        -1, -1, -1,
        NULL, NULL,
    },
};

static const pci_subclass_t bridg_subclass[] = {
    {
        0x00, "PCI host bridge",           NULL,  hbrg_devices, NULL,
        NULL, NULL,
    },
    {
        0x01, "ISA bridge",                NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x02, "EISA bridge",               NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x03, "MCA bridge",                NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x04, "PCI-to-PCI bridge",         NULL,  PCIbrg_devices, NULL,
        NULL, NULL,
    },
    {
        0x05, "PCMCIA bridge",             NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x06, "NUBUS bridge",              NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x07, "cardbus bridge",            NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x08, "raceway bridge",            NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x09, "semi-transparent PCI-to-PCI bridge", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x0A, "infiniband-to-PCI bridge",  NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0x80, "misc PCI bridge",           NULL,  NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_iface_t serial_iface[] = {
    {
        0x00, "XT serial controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x01, "16450 serial controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x02, "16550 serial controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x03, "16650 serial controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x04, "16750 serial controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x05, "16850 serial controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x06, "16950 serial controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_iface_t par_iface[] = {
    {
        0x00, "parallel port", NULL,
        NULL, NULL, NULL,
    },
    {
        0x01, "bi-directional parallel port", NULL,
        NULL, NULL, NULL,
    },
    {
        0x02, "ECP 1.x parallel port", NULL,
        NULL, NULL, NULL,
    },
    {
        0x03, "IEEE 1284 controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFE, "IEEE 1284 device", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_iface_t modem_iface[] = {
    {
        0x00, "generic modem", NULL,
        NULL, NULL, NULL,
    },
    {
        0x01, "Hayes 16450 modem", NULL,
        NULL, NULL, NULL,
    },
    {
        0x02, "Hayes 16550 modem", NULL,
        NULL, NULL, NULL,
    },
    {
        0x03, "Hayes 16650 modem", NULL,
        NULL, NULL, NULL,
    },
    {
        0x04, "Hayes 16750 modem", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_subclass_t comm_subclass[] = {
    {
        0x00, "serial controller",          NULL, NULL, serial_iface,
        NULL, NULL,
    },
    {
        0x01, "parallel port",             NULL, NULL, par_iface,
        NULL, NULL,
    },
    {
        0x02, "multiport serial controller", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x03, "modem",                     NULL, NULL, modem_iface,
        NULL, NULL,
    },
    {
        0x04, "GPIB controller",           NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x05, "smart card",                NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x80, "misc communication device", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL, NULL, NULL,
        NULL, NULL,
    },
};

static const pci_iface_t pic_iface[] = {
    {
        0x00, "8259 PIC", NULL,
        NULL, NULL, NULL,
    },
    {
        0x01, "ISA PIC", NULL,
        NULL, NULL, NULL,
    },
    {
        0x02, "EISA PIC", NULL,
        NULL, NULL, NULL,
    },
    {
        0x10, "I/O APIC", NULL,
        NULL, NULL, NULL,
    },
    {
        0x20, "I/O APIC", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_iface_t dma_iface[] = {
    {
        0x00, "8237 DMA controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x01, "ISA DMA controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x02, "EISA DMA controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_iface_t tmr_iface[] = {
    {
        0x00, "8254 system timer", NULL,
        NULL, NULL, NULL,
    },
    {
        0x01, "ISA system timer", NULL,
        NULL, NULL, NULL,
    },
    {
        0x02, "EISA system timer", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_iface_t rtc_iface[] = {
    {
        0x00, "generic RTC controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x01, "ISA RTC controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_dev_t sys_devices[] = {
    /* IBM MPIC controller */
    {
        0x1014, 0x0002,
        "open-pic", "MPIC", NULL, "chrp,open-pic",
        0, 0, 2,
        NULL, NULL,
    },
    /* IBM MPIC2 controller */
    {
        0x1014, 0xFFFF,
        "open-pic", "MPIC2", NULL, "chrp,open-pic",
        0, 0, 2,
        NULL, NULL,
    },
    {
        0xFFFF, 0xFFFF,
        NULL, NULL, NULL, NULL,
        -1, -1, -1,
        NULL, NULL,
    },
};

static const pci_subclass_t sys_subclass[] = {
    {
        0x00, "PIC",                       NULL, NULL, pic_iface,
        NULL, NULL,
    },
    {
        0x01, "DMA controller",             NULL, NULL, dma_iface,
        NULL, NULL,
    },
    {
        0x02, "system timer",              NULL, NULL, tmr_iface,
        NULL, NULL,
    },
    {
        0x03, "RTC controller",             NULL, NULL, rtc_iface,
        NULL, NULL,
    },
    {
        0x04, "PCI hotplug controller",     NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x80, "misc system peripheral",    NULL, sys_devices, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_subclass_t inp_subclass[] = {
    {
        0x00, "keyboard controller",        NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x01, "digitizer",                 NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x02, "mouse controller",           NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x03, "scanner controller",         NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x04, "gameport controller",        NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x80, "misc input device",         NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_subclass_t dock_subclass[] = {
    {
        0x00, "generic docking station",   NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x80, "misc docking station",      NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_subclass_t cpu_subclass[] = {
    {
        0x00, "i386 processor",            NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x01, "i486 processor",            NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x02, "pentium processor",         NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x10, "alpha processor",           NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x20, "PowerPC processor",         NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x30, "MIPS processor",            NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x40, "co-processor",              NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_iface_t usb_iface[] = {
    {
        0x00, "UHCI USB controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x10, "OHCI USB controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x20, "EHCI USB controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0x80, "misc USB controller", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFE, "USB device", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_iface_t ipmi_iface[] = {
    {
        0x00, "IPMI SMIC interface", NULL,
        NULL, NULL, NULL,
    },
    {
        0x01, "IPMI keyboard interface", NULL,
        NULL, NULL, NULL,
    },
    {
        0x02, "IPMI block transfer interface", NULL,
        NULL, NULL, NULL,
    },
    {
        0xFF, NULL, NULL,
        NULL, NULL, NULL,
    },
};

static const pci_subclass_t ser_subclass[] = {
    {
        0x00, "Firewire bus controller",    "ieee1394", NULL, NULL,
        NULL, NULL,
    },
    {
        0x01, "ACCESS bus controller",      NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x02, "SSA controller",             NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x03, "USB controller",             "usb", NULL, usb_iface,
        NULL, NULL,
    },
    {
        0x04, "fibre channel controller",   NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x05, "SMBus controller",           NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x06, "InfiniBand controller",      NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x07, "IPMI interface",            NULL, NULL,  ipmi_iface,
        NULL, NULL,
    },
    {
        0x08, "SERCOS controller",          NULL, NULL,  ipmi_iface,
        NULL, NULL,
    },
    {
        0x09, "CANbus controller",          NULL, NULL,  ipmi_iface,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_subclass_t wrl_subclass[] = {
    {
        0x00, "IRDA controller",           NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x01, "consumer IR controller",    NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x10, "RF controller",             NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x11, "bluetooth controller",      NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x12, "broadband controller",      NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x80, "misc wireless controller",  NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_subclass_t sat_subclass[] = {
    {
        0x01, "satellite TV controller",   NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x02, "satellite audio controller", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x03, "satellite voice controller", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x04, "satellite data controller", NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_subclass_t crypt_subclass[] = {
    {
        0x00, "cryptographic network controller", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x10, "cryptographic entertainment controller", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x80, "misc cryptographic controller",    NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_subclass_t spc_subclass[] = {
    {
        0x00, "DPIO module",               NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x01, "performances counters",     NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x10, "communication synchronisation", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0x20, "management card",           NULL, NULL,  NULL,
        NULL, NULL,
    },
    {
        0x80, "misc signal processing controller", NULL, NULL, NULL,
        NULL, NULL,
    },
    {
        0xFF, NULL,                        NULL,  NULL,  NULL,
        NULL, NULL,
    },
};

static const pci_class_t pci_classes[] = {
    /* 0x00 */
    { "undefined",                         NULL,             undef_subclass, },
    /* 0x01 */
    { "mass-storage controller",           NULL,              mass_subclass, },
    /* 0x02 */
    { "network controller",                "network",          net_subclass, },
    /* 0x03 */
    { "display controller",                "display",        displ_subclass, },
    /* 0x04 */
    { "multimedia device",                 NULL,             media_subclass, },
    /* 0x05 */
    { "memory controller",                 "memory-controller", mem_subclass, },
    /* 0x06 */
    { "PCI bridge",                        "pci",            bridg_subclass, },
    /* 0x07 */
    { "communication device",              NULL,               comm_subclass,},
    /* 0x08 */
    { "system peripheral",                 NULL,               sys_subclass, },
    /* 0x09 */
    { "input device",                      NULL,               inp_subclass, },
    /* 0x0A */
    { "docking station",                   NULL,              dock_subclass, },
    /* 0x0B */
    { "processor",                         NULL,               cpu_subclass, },
    /* 0x0C */
    { "serial bus controller",             NULL,               ser_subclass, },
    /* 0x0D */
    { "wireless controller",               NULL,               wrl_subclass, },
    /* 0x0E */
    { "intelligent I/O controller",        NULL,               NULL,         },
    /* 0x0F */
    { "satellite communication controller", NULL,               sat_subclass, },
    /* 0x10 */
    { "cryptographic controller",           NULL,             crypt_subclass, },
    /* 0x11 */
    { "signal processing controller",       NULL,               spc_subclass, },
};

phandle_t pic_handle;
static int macio_config_cb (const pci_config_t *config)
{
#ifdef CONFIG_PPC
	char buf[64];
	phandle_t ph;
        cell props[2];

        snprintf(buf, sizeof(buf), "%s/interrupt-controller", config->path);
	REGISTER_NAMED_NODE(ob_pci_node, buf);

	ph = find_dev(buf);
	set_property(ph, "device_type", "interrupt-controller", 21);
	set_property(ph, "compatible", "heathrow\0mac-risc", 18);
	set_int_property(ph, "#interrupt-cells", 1);
	props[0]= 0x10;
	props[1]= 0x20;
	set_property(ph, "reg", &props, sizeof(props));
	pic_handle = ph;

	cuda_init(config->path, config->regions[0]);
	macio_nvram_init(config->path, config->regions[0]);
#endif
	return 0;
}

static const pci_dev_t misc_pci[] = {
    /* Paddington Mac I/O */
    {
        0x106B, 0x0017,
        "mac-io", "mac-io", "AAPL,343S1211", "paddington\1heathrow",
        1, 1, 1,
        &macio_config_cb, NULL,
    },
    /* KeyLargo Mac I/O */
    {
        0x106B, 0x0022,
        "mac-io", "mac-io", "AAPL,Keylargo", "Keylargo",
        1, 1, 2,
        &macio_config_cb, NULL,
    },
    {
        0xFFFF, 0xFFFF,
        NULL, NULL, NULL, NULL,
        -1, -1, -1,
        NULL, NULL,
    },
};

static const pci_dev_t *pci_find_device (uint8_t class, uint8_t subclass,
                                         uint8_t iface, uint16_t vendor,
                                         uint16_t product)
{
    int (*config_cb)(pci_config_t *config);
    const pci_class_t *pclass;
    const pci_subclass_t *psubclass;
    const pci_iface_t *piface;
    const pci_dev_t *dev;
    const void *private;
    pci_dev_t *new;
    const char *name, *type;

    name = "unknown";
    type = "unknown";
    config_cb = NULL;
    private = NULL;

    if (class == 0x00 && subclass == 0x01) {
        /* Special hack for old style VGA devices */
        class = 0x03;
        subclass = 0x00;
    } else if (class == 0xFF) {
        /* Special case for misc devices */
        dev = misc_pci;
        goto find_device;
    }
    if (class > (sizeof(pci_classes) / sizeof(pci_class_t))) {
        name = "invalid PCI device";
        type = "invalid";
        goto bad_device;
    }
    pclass = &pci_classes[class];
    name = pclass->name;
    type = pclass->type;
    for (psubclass = pclass->subc; ; psubclass++) {
        if (psubclass->subclass == 0xFF)
            goto bad_device;
        if (psubclass->subclass == subclass) {
            if (psubclass->name != NULL)
                name = psubclass->name;
            if (psubclass->type != NULL)
                type = psubclass->type;
            if (psubclass->config_cb != NULL) {
                config_cb = psubclass->config_cb;
            }
            if (psubclass->private != NULL)
                private = psubclass->private;
            if (psubclass->iface != NULL)
                break;
            dev = psubclass->devices;
            goto find_device;
        }
    }
    for (piface = psubclass->iface; ; piface++) {
        if (piface->iface == 0xFF) {
            dev = psubclass->devices;
            break;
        }
        if (piface->iface == iface) {
            if (piface->name != NULL)
                name = piface->name;
            if (piface->type != NULL)
                type = piface->type;
            if (piface->config_cb != NULL) {
                config_cb = piface->config_cb;
            }
            if (piface->private != NULL)
                private = piface->private;
            dev = piface->devices;
            break;
        }
    }
    find_device:
    for (;; dev++) {
        if (dev->vendor == 0xFFFF && dev->product == 0xFFFF) {
            goto bad_device;
        }
        if (dev->vendor == vendor && dev->product == product) {
            if (dev->name != NULL)
                name = dev->name;
            if (dev->type != NULL)
                type = dev->type;
            if (dev->config_cb != NULL) {
                config_cb = dev->config_cb;
            }
            if (dev->private != NULL)
                private = dev->private;
            new = malloc(sizeof(pci_dev_t));
            if (new == NULL)
                return NULL;
            new->vendor = vendor;
            new->product = product;
            new->type = type;
            new->name = name;
            new->model = dev->model;
            new->compat = dev->compat;
            new->config_cb = config_cb;
            new->private = private;

            return new;
        }
    }
 bad_device:
    printk("Cannot manage '%s' PCI device type '%s':\n %x %x (%x %x %x)\n",
           name, type, vendor, product, class, subclass, iface);

    return NULL;
}

#define set_bool_property(ph, name) set_property(ph, name, NULL, 0);


static void ob_pci_add_properties(pci_addr addr, const pci_dev_t *pci_dev,
                                  const pci_config_t *config)
{
	phandle_t dev=get_cur_dev();
	int status,id;
	uint16_t vendor_id, device_id;
	uint8_t rev;

	vendor_id = pci_config_read16(addr, PCI_VENDOR_ID);
	device_id = pci_config_read16(addr, PCI_DEVICE_ID);
	rev = pci_config_read8(addr, PCI_REVISION_ID);

	/* create properties as described in 2.5 */

	printk("%s\n", pci_dev->name);
	set_int_property(dev, "vendor-id", vendor_id);
	set_int_property(dev, "device-id", device_id);
	set_int_property(dev, "revision-id", rev);

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
	push_str(pci_dev->name);
	fword("encode-string");
	push_str("class");
	fword("property");
	if (pci_dev->config_cb)
		pci_dev->config_cb(config);
}


static void ob_pci_add_reg(pci_addr addr)
{
	PUSH(0);
	PUSH(0);
	PUSH(addr&(~arch->cfg_base));
	fword("pci-addr-encode");
	PUSH(0);
	PUSH(0);
	fword("pci-len-encode");
	fword("encode+");
	push_str("reg");
	fword("property");
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
ob_pci_configure(pci_arch_t *addr, pci_config_t *config, uint32_t *mem_base,
                 uint32_t *io_base)

{
	uint32_t smask, omask, amask, size, reloc, min_align;
	uint32_t *base;
	pci_addr config_addr;
	int reg;

	omask = 0x00000000;
	for (reg = 0; reg < 7; reg++) {

		config->regions[reg] = 0x00000000;
		config->sizes[reg] = 0x00000000;

		if ((omask & 0x0000000f) == 0x4) {
			/* 64 bits memory mapping */
			continue;
		}

		if (reg == 6)
			config_addr = PCI_ROM_ADDRESS;
		else
			config_addr = PCI_BASE_ADDR_0 + reg * 4;

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
		config->regions[reg] = reloc;
printk("region %08x size %08x\n", config->regions[reg], config->sizes[reg]);
	}
}

static void ob_scan_pci_bus(int bus, unsigned long *mem_base,
                            unsigned long *io_base, const char *path)
{
	int devnum, fn, is_multi, vid, did;
	unsigned int htype;
	pci_addr addr;
	phandle_t dnode, dbus;
	pci_config_t config;
        const pci_dev_t *pci_dev;
	uint32_t ccode;
	uint8_t class, subclass, iface, rev;

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

			pci_dev = pci_find_device(class, subclass, iface, vid, did);

#ifdef CONFIG_DEBUG_PCI
			printk("%x:%x.%x - %x:%x - ", bus, devnum, fn,
					vid, did);
#endif
			htype = pci_config_read8(addr, PCI_HEADER_TYPE);
			if (fn == 0)
				is_multi = htype & 0x80;

			activate_device(path);

			dbus=get_cur_dev();
			if (pci_dev == NULL || pci_dev->name == NULL)
                            snprintf(config.path, sizeof(config.path),
                                     "%s/pci%x,%x", path, vid, did);
			else
                            snprintf(config.path, sizeof(config.path),
                                     "%s/%s", path, pci_dev->name);
#ifdef CONFIG_DEBUG_PCI
			printk("%s - ", config.path);
#endif
			REGISTER_NAMED_NODE(ob_pci_node, config.path);
			dnode=find_dev(config.path);
			activate_dev( dnode );
                        ob_pci_configure(addr, &config, mem_base, io_base);
{
	int irq_pin, irq_line;
	static const uint8_t heathrow_pci_irqs[4] = { 0x15, 0x16, 0x17, 0x18 };
	irq_pin = pci_config_read8(addr, PCI_INTERRUPT_PIN);
	if (irq_pin > 0) {
		irq_pin = (devnum + irq_pin - 1) & 3;
		irq_line = heathrow_pci_irqs[irq_pin];
	}
}
			ob_pci_add_properties(addr, pci_dev, &config);
			ob_pci_add_reg(addr);
			device_end();

		}
	}
}

int ob_pci_init(void)
{
        int bus;
        unsigned long mem_base, io_base;

	printk("Initializing PCI devices...\n");

	/* brute force bus scan */

	/* Find all PCI bridges */

	mem_base = arch->mem_base;
	io_base = arch->io_base;
	for (bus = 0; bus<0x100; bus++) {
		ob_scan_pci_bus(bus, &mem_base, &io_base, "/pci");
	}
	return 0;
}
