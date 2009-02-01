#ifndef _H_PCI
#define _H_PCI

typedef uint32_t pci_addr;

typedef struct pci_arch_t pci_arch_t;

struct pci_arch_t {
	const char * name;
	uint16_t vendor_id;
	uint16_t device_id;
	unsigned long cfg_addr;
	unsigned long cfg_data;
	unsigned long cfg_base;
	unsigned long cfg_len;
	unsigned long mem_base;
	unsigned long mem_len;
	unsigned long io_base;
	unsigned long io_len;
	unsigned long rbase;
	unsigned long rlen;
	uint8_t irqs[4];
};

extern const pci_arch_t *arch;

#define PCI_VENDOR_ID_ATI                0x1002
#define PCI_DEVICE_ID_ATI_RAGE128_PF     0x5046

#define PCI_VENDOR_ID_DEC                0x1011
#define PCI_DEVICE_ID_DEC_21154          0x0026

#define PCI_VENDOR_ID_IBM                0x1014
#define PCI_DEVICE_ID_IBM_OPENPIC        0x0002
#define PCI_DEVICE_ID_IBM_OPENPIC2       0xffff

#define PCI_VENDOR_ID_MOTOROLA           0x1057
#define PCI_DEVICE_ID_MOTOROLA_MPC106    0x0002
#define PCI_DEVICE_ID_MOTOROLA_RAVEN     0x4801

#define PCI_VENDOR_ID_APPLE              0x106b
#define PCI_DEVICE_ID_APPLE_343S1201     0x0010
#define PCI_DEVICE_ID_APPLE_343S1211     0x0017
#define PCI_DEVICE_ID_APPLE_UNI_N_I_PCI  0x001e
#define PCI_DEVICE_ID_APPLE_UNI_N_PCI    0x001f
#define PCI_DEVICE_ID_APPLE_UNI_N_AGP    0x0020
#define PCI_DEVICE_ID_APPLE_UNI_N_KEYL   0x0022

#define PCI_VENDOR_ID_SUN                0x108e
#define PCI_DEVICE_ID_SUN_EBUS           0x1000
#define PCI_DEVICE_ID_SUN_SIMBA          0x5000
#define PCI_DEVICE_ID_SUN_PBM            0x8000
#define PCI_DEVICE_ID_SUN_SABRE          0xa000

#define PCI_VENDOR_ID_CMD                0x1095
#define PCI_DEVICE_ID_CMD_646            0x0646

#define PCI_VENDOR_ID_REALTEK            0x10ec
#define PCI_DEVICE_ID_REALTEK_RTL8029    0x8029

#define PCI_VENDOR_ID_QEMU               0x1234
#define PCI_DEVICE_ID_QEMU_VGA           0x1111

#define PCI_VENDOR_ID_INTEL              0x8086
#define PCI_DEVICE_ID_INTEL_82441        0x1237

#endif	/* _H_PCI */
