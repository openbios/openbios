#include "openbios/config.h"
#include "openbios/bindings.h"
#include "libc/vsprintf.h"

#include "pci_database.h"

/* PCI devices database */

typedef struct pci_class_t pci_class_t;
typedef struct pci_subclass_t pci_subclass_t;
typedef struct pci_iface_t pci_iface_t;

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

/* Current machine description */

static const pci_subclass_t undef_subclass[] = {
    {
        0xFF, NULL, NULL, NULL, NULL,
        NULL, NULL,
    },
};

static const pci_dev_t ide_devices[] = {
    {
        0x1095, 0x0646, /* CMD646 IDE controller */
        "pci-ide", "pci-ata", NULL,
	"pci1095,646\0pci1095,646\0pciclass,01018f\0",
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
        NULL, "ATY",      "ATY Rage128", "VGA\0",
        0, 0, 0,
        NULL, NULL,
    },
    {
        0x1234, 0x1111,
        NULL, "QEMU,VGA", "Qemu VGA",    "VGA\0",
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


static const pci_dev_t hbrg_devices[] = {
    {
        0x106B, 0x0020, NULL,
        "pci", "AAPL,UniNorth", "uni-north\0",
        3, 2, 1,
        NULL, NULL
    },
    {
        0x106B, 0x001F, NULL,
        "pci", "AAPL,UniNorth", "uni-north\0",
        3, 2, 1,
        NULL, NULL
    },
    {
        0x106B, 0x001E, NULL,
        "pci", "AAPL,UniNorth", "uni-north\0",
        3, 2, 1,
        NULL, NULL
    },
    {
        0x1057, 0x0002, "pci",
        "pci", "MOT,MPC106", "grackle\0",
        3, 2, 1,
        host_config_cb, NULL
    },
    {
        0x1057, 0x4801, NULL,
        "pci-bridge", "PREP Host PCI Bridge - Motorola Raven", NULL,
        3, 2, 1,
        NULL, NULL,
    },
    {
        0x108e, 0xa000, NULL,
        "pci", "SUNW,simba", "pci108e,a000\0pciclass,0\0",
        3, 2, 1,
        host_config_cb, NULL,
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
        "pci-bridge", "DEV,21154", "DEV,21154\0pci-bridge\0",
        3, 2, 1,
        bridge_config_cb, NULL,
    },
    {
        0x108e, 0x5000, NULL,
        "pci", "SUNW,sabre", "pci108e,5000\0pciclass,060400\0",
        3, 2, 1,
        bridge_config_cb, NULL,
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
        "open-pic", "MPIC", NULL, "chrp,open-pic\0",
        0, 0, 2,
        NULL, NULL,
    },
    /* IBM MPIC2 controller */
    {
        0x1014, 0xFFFF,
        "open-pic", "MPIC2", NULL, "chrp,open-pic\0",
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

static const pci_dev_t misc_pci[] = {
    /* Heathrow Mac I/O */
    {
        0x106B, 0x0010,
        "mac-io", "mac-io", "AAPL,343S1201", "heathrow\0",
        1, 1, 1,
        &macio_config_cb, NULL,
    },
    /* Paddington Mac I/O */
    {
        0x106B, 0x0017,
        "mac-io", "mac-io", "AAPL,343S1211", "paddington\0heathrow\0",
        1, 1, 1,
        &macio_config_cb, NULL,
    },
    /* KeyLargo Mac I/O */
    {
        0x106B, 0x0022,
        "mac-io", "mac-io", "AAPL,Keylargo", "Keylargo\0",
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

const pci_dev_t *pci_find_device (uint8_t class, uint8_t subclass,
                                  uint8_t iface, uint16_t vendor,
                                  uint16_t product)
{
    int (*config_cb)(const pci_config_t *config);
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
    if (dev == NULL)
	goto bad_device;
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
            new->acells = dev->acells;
            new->scells = dev->scells;
            new->icells = dev->icells;
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
