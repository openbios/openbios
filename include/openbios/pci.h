#ifndef _H_PCI
#define _H_PCI

typedef struct pci_arch_t pci_arch_t;

struct pci_arch_t {
	char * name;
	uint16_t vendor_id;
	uint16_t device_id;
	uint32_t cfg_addr;
	uint32_t cfg_data;
	uint32_t cfg_base;
	uint32_t cfg_len;
	uint32_t mem_base;
	uint32_t mem_len;
	uint32_t io_base;
	uint32_t io_len;
	uint32_t rbase;
	uint32_t rlen;
};

#endif	/* _H_PCI */
